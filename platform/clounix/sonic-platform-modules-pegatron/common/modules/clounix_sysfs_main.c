#include "clounix_sysfs_common.h"
#include "clounix_sysfs_var.h"

static struct kobject *clounix_switch;
static struct kernfs_node *fail_node = NULL;
atomic_t is_work = ATOMIC_INIT(0);

LIST_HEAD(clounix_list_head);
DEFINE_SPINLOCK(node_rw_lock);

static inline int in_work(void)
{
    if (atomic_read(&is_work) < 0)
        return 1;

    if (atomic_add_negative(1, &is_work) != 0) {
        atomic_dec(&is_work);
        return 1;
    }

    return 0;
}

static inline void out_work(void)
{
    atomic_dec(&is_work); 
}

static char *skip_root(char *path)
{
    if (memcmp(path, SYSFS_ROOT, strlen(SYSFS_ROOT)) == 0) {
        path += strlen(SYSFS_ROOT);
        return path;
    }

    return NULL;
}

static void traverse_sysfs_by_rbtree(struct kernfs_node *parent)
{
    struct rb_node *node;
    struct kernfs_node *sd;

    if (parent == NULL)
        return;

    node = rb_first(&(parent->dir.children));

    while (node != NULL) {
        sd = container_of(node, struct kernfs_node, rb);
        printk(KERN_DEBUG "node name: %s \t type: %x parent dir %s\r\n", sd->name ? sd->name : "null", sd->flags, sd->parent->name);
        node = rb_next(node);
    }
}

static struct kernfs_node *search_sysfs_node_by_path(struct kernfs_node *root, char *path)
{
    char *start, *end;
    struct kernfs_node *sd = root;

    if (path == NULL || root == NULL)
        goto out;

    if ((start = skip_root(path)) == NULL)
        start = path;
    
    while (start != NULL) {
        while (*start == '/')
            start++;
        
        if ((end = strchr(start, '/')) != NULL) {
            *end = '\0';
            sd = sysfs_get_dirent(sd, start);
            sysfs_put(sd);
            *end = '/';

            if (sd == NULL)
                goto out;

            if ((sd->flags & KERNFS_LINK) != 0)
                sd = sd->symlink.target_kn;

            start = end+1;
        } else {
            sd = sysfs_get_dirent(sd, start);
            sysfs_put(sd);
            break;
        }
    }
  
    return sd;

out:
    return NULL; 
}

static struct kernfs_node *pass_path(struct kobject **kobj, struct kernfs_node *root, char *path, char **name)
{
    struct kernfs_node *sd = NULL;
    char *start, *end;

    if ((start = skip_root(path)) == NULL) {
        printk(KERN_DEBUG "path format err\r\n");
        return NULL;
    }

    while (start != NULL) {
        while (*start == '/')
            start++;

        if ((end = strchr(start, '/')) != NULL) {
            *end = '\0';
            if ((sd = sysfs_get_dirent(root, start)) == NULL) {
                if ((*kobj = kobject_create_and_add(start, root->priv)) == NULL)
                    goto path_err;

                printk(KERN_DEBUG "create missing dir %s\r\n", (*kobj)->sd->name);
                sd = (*kobj)->sd;
                sysfs_get(sd);
            }

            sysfs_put(sd);
            *end = '/';
            root = sd;
            start = end+1;
            continue;
        }
        *kobj = root->priv;
        *name = start;
        break;
    }

    return root;

path_err:
    return NULL;
}

static int create_link_auto(struct kernfs_node *root, struct kernfs_node *target, char *path)
{
    struct kernfs_node *sd;
    struct kobject *kobj = NULL;
    struct kobject fake_kobj = {0};
    char *start;
    int err_no;

    if ((root = pass_path(&kobj, root, path, &start)) == NULL)
        return -ENOMEM;

    if ((sd = sysfs_get_dirent(root, start)) != NULL) {
        sysfs_put(sd);
        printk(KERN_DEBUG "link exist\r\n");
        return -EEXIST;
    }
    /*
    Some sysfs node is create by func 'sysfs_create_group(s)', the node's kernfs_node struct's
    member 'priv' is 'attribute' not 'kobject'.Can use fake kobj to create link, because the
    'sysfs_crate_link' only use kobj->sd as link source and never use other member in kobj.
    */
    fake_kobj.sd = target;

    if ((err_no = sysfs_create_link(kobj, &fake_kobj, start)) != 0) {
        printk(KERN_DEBUG "fail to create link %s in dir %s\r\n", start, kobj->sd->name);
        return err_no;  
    }

    return 0;
}

static char *format_check(const char *buf)
{
    char *p = (char *)buf;

    if ((p = strchr(p, ' ')) != NULL) {
        if (*(p+1) != ' ')
            return p;
        else
            return NULL;
    }

    return NULL;
}

static struct clounix_node_info *find_node_from_list(struct attribute *attr)
{
    struct clounix_node_info *pos = NULL;
    struct clounix_node_info *tmp = NULL;

    list_for_each_entry_safe(pos, tmp, &clounix_list_head, node) {
        if (attr == &(pos->kobj_attr.attr))
            return pos;
    }
    
    return NULL;
}

static void rm_all_node(void)
{
    struct clounix_node_info *pos = NULL;
    struct clounix_node_info *tmp = NULL;

    spin_lock(&node_rw_lock);
    list_for_each_entry_safe(pos, tmp, &clounix_list_head, node) {
        list_del_init(&(pos->node));
        kfree(pos);
    }
    spin_unlock(&node_rw_lock);
}

static ssize_t node_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    struct clounix_node_info *pos = NULL;
    int len = 0;

    spin_lock(&node_rw_lock);
    if ((pos = find_node_from_list(&(attr->attr))) != NULL)
        len = sprintf(buf, "%s\r\n", pos->info);
    spin_unlock(&node_rw_lock);
    
    return len;
}

static int add_node(struct kernfs_node *root, char *buf)
{
    int err_no;
    char *node_path, *info;
    struct kobject *kobj;
    struct kernfs_node *tmp;
    struct clounix_node_info *attr_node;

    node_path = format_check(buf);
    if (node_path == NULL)
        return -ENOENT;

    info = node_path + 1;
    *node_path = '\0';
    node_path = buf;
   
    if ((root = pass_path(&kobj, root, node_path, &node_path)) == NULL) 
        return -ENOENT;

    if (strlen(node_path) > MAX_NODE_NAME_LEN)
        return -ENOMEM;

    if (strlen(info) > MAX_NODE_INFO_LEN)
        return -ENOMEM;

    if ((tmp = sysfs_get_dirent(root, node_path)) != NULL) {
        sysfs_put(tmp);
        printk(KERN_DEBUG "node exist\r\n");
        return -EEXIST;
    }

    if ((attr_node = kzalloc(sizeof(struct clounix_node_info), GFP_ATOMIC)) == NULL)
        return -ENOMEM;

    strcpy(attr_node->name, node_path);
    strcpy(attr_node->info, info);
    attr_node->kobj_attr.attr.name = attr_node->name;
    attr_node->kobj_attr.attr.mode = VERIFY_OCTAL_PERMISSIONS(0444);
    attr_node->kobj_attr.show = node_show;
    attr_node->kobj_attr.store = NULL;

    if ((err_no = sysfs_create_file(kobj, &(attr_node->kobj_attr.attr))) != 0) {
        kfree(attr_node);
        return err_no;
    }

    spin_lock(&node_rw_lock);
    list_add_tail(&(attr_node->node), &clounix_list_head);
    spin_unlock(&node_rw_lock);
    
    return 0;
}

static int del_node(struct kernfs_node *root, char *link)
{
    struct kernfs_node *sd;
    struct clounix_node_info *node = NULL;
 
    if ((sd = search_sysfs_node_by_path(root, link)) == NULL)
        goto not_find;

    if ((sd->flags & KERNFS_LINK) != 0)
        sysfs_remove_link(sd->parent->priv,  sd->name);
    else if ((sd->flags & KERNFS_DIR) != 0)
        kobject_del(sd->priv);
    else if ((sd->flags & KERNFS_FILE) != 0) {
        spin_lock(&node_rw_lock);
        if ((node = find_node_from_list(sd->priv)) == NULL) {
            spin_unlock(&node_rw_lock);
            goto not_find;
        }
        list_del_init(&node->node);
        spin_unlock(&node_rw_lock);

        sysfs_remove_file(sd->parent->priv, sd->priv);
        kfree(node);
    }

    return 0;

not_find:
    printk(KERN_DEBUG "del target not found\r\n");
    return -1;
}

static int cmd_guess(const char *buf)
{
    if (memcmp(buf, DEL_CMD, strlen(DEL_CMD)) == 0)
        return DEL_OPS;
    else if (memcmp(buf, ADD_CMD, strlen(ADD_CMD)) == 0)
        return ADD_OPS;
    else
        return 0;
}

static int add_new_link(struct kernfs_node *root, char *src, char *node)
{
    struct kernfs_node *target = NULL;

    if ((target = search_sysfs_node_by_path(root, src)) == NULL)
        goto path_err;

    if ((target->flags & KERNFS_LINK) != 0)
        target = target->symlink.target_kn;

    create_link_auto(root, target, node);

    return 0;

path_err:
    printk(KERN_DEBUG "not find sysfs node: %s\r\n", src);
    return -1;
}

static ssize_t cmd_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    traverse_sysfs_by_rbtree(fail_node);

    return 0;
}

static ssize_t cmd_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    struct kernfs_node *root = clounix_switch->sd->parent;
    char *src, *pra;
    char *p;

    if (in_work() == 1)
		return count;

    if (count <= MIN_PATH_LEN || count > PAGE_SIZE)
        goto format_err;

    if ((p = format_check(buf)) == NULL)
        goto format_err;

    if ((src = kzalloc(PAGE_SIZE, GFP_ATOMIC)) == NULL)
        goto format_err;
        
    memcpy(src, buf, count-1);
    pra = src + (p - buf);
    *pra = '\0';
    pra++;

    switch (cmd_guess(buf)) {
    case DEL_OPS:
        if (del_node(root, pra) != 0)
            goto path_err;
        break;
    case ADD_OPS:
        if (add_node(root, pra) != 0)
            goto path_err;
        break;
    default:
        if (add_new_link(root, src, pra) != 0)
            goto path_err;
        break;
    }

    kfree(src);
    out_work();
    return count;

path_err:
    printk(KERN_DEBUG "not find %s\r\n", buf);
format_err:
    printk(KERN_DEBUG KERN_ALERT PATH_FORMAT); 
    kfree(src);
    out_work();
    return count;
}

static struct kobj_attribute ctrl_attr = __ATTR(clounix_cmd, 0644, cmd_show, cmd_store);

static struct attribute * ctrl_attrs[] = {
    &ctrl_attr.attr,
    NULL
};

static const struct attribute_group control_attr_group = {
    .attrs = ctrl_attrs,
};

static int __init main_init(void)
{
    int err_no = 0;

    printk(KERN_DEBUG "clounix kobj node_init!\r\n");

    clounix_switch = kobject_create_and_add(CLOUNIX_DIR_NAME, NULL);
    if (clounix_switch == NULL)
        return -ENOMEM;

    if ((err_no = sysfs_create_group(clounix_switch, &control_attr_group)) != 0)
        goto err_out;

    return 0;

err_out:
    printk(KERN_DEBUG "has err_no %d\r\n", err_no);
    kobject_put(clounix_switch);
    return -ENOMEM;
}

static void __exit main_exit(void)
{
    printk(KERN_DEBUG "clounix kobj node_del!\r\n");

    if (atomic_sub_return(2, &is_work) != -2) {
        while (atomic_read(&is_work) != -2) {};
    }
    sysfs_remove_group(clounix_switch, &control_attr_group);

    rm_all_node();
    kobject_del(clounix_switch);
}

module_init(main_init);
module_exit(main_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("baohx@clounix.com");
MODULE_DESCRIPTION("clounix sysfs operate interface");
