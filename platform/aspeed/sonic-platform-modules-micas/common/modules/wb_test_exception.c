/*
 *
 * test_exception.c
 * Original Author: 2023-06-13
 *
 *
 * History
 *  [Version]        [Author]               [Date]            [Description]
 *    v1.0             xxx                 2023-06-26         Initial version
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/version.h>
#include <linux/timer.h>
#include <linux/uaccess.h>
#include <linux/synclink.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/fs.h>


#define PROC_FILE_NAME            "wb_test_exception"
#define USR_PARA_MAX_SIZE        (16)

enum test_type_e {
    TEST_TYPE_BUG = 0,
    TEST_TYPE_PANIC,
    TEST_TYPE_OOPS,
    TEST_TYPE_BUG_INT,
    TEST_TYPE_PANIC_INT,
    TEST_TYPE_OOPS_INT,
    TEST_TYPE_END,
    TEST_TYPE_INVAILD,
};

static int debug = 0;
module_param(debug, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(debug, "control print info level. 0: none; 1: err;" \
                             "2: err + warn; 3: err + warn +info; >3: all");
#define DEBUG_ERROR(fmt, args...)                                                           \
    do {                                                                                    \
        if (debug > 0) {                                                                    \
            printk(KERN_ERR "[func:%s line:%d]  "fmt, __func__, __LINE__, ## args);         \
        } else {                                                                            \
            pr_debug(fmt, ## args);                                                         \
        }                                                                                   \
    } while(0)

#define DEBUG_WARN(fmt, args...)                                                            \
        do {                                                                                \
            if (debug > 1) {                                                                \
                printk(KERN_WARNING "[func:%s line:%d]  "fmt, __func__, __LINE__, ## args); \
            } else {                                                                        \
                pr_debug(fmt, ## args);                                                     \
            }                                                                               \
        } while(0)

#define DEBUG_INFO(fmt, args...)                                                            \
    do {                                                                                    \
        if (debug > 2) {                                                                    \
            printk(KERN_INFO "[func:%s line:%d]  "fmt, __func__, __LINE__, ## args);        \
        } else {                                                                            \
            pr_debug(fmt, ## args);                                                         \
        }                                                                                   \
    } while(0)

#define DEBUG_VERBOSE(fmt, args...)                                                         \
    do {                                                                                    \
        if (debug > 3) {                                                                    \
            printk(KERN_DEBUG "[func:%s line:%d]  "fmt, __func__, __LINE__, ## args);       \
        } else {                                                                            \
            pr_debug(fmt, ## args);                                                         \
        }                                                                                   \
    } while(0)

static struct timer_list g_test_exception_timer;
static unsigned long g_test_type;


static void create_oops(void)
{
    char *killer = NULL;
    *killer = 1;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 15, 0)
static void test_exception_timer_cb(unsigned long arg)
#else
static void test_exception_timer_cb(struct timer_list *timer)
#endif
{
    DEBUG_INFO("g_test_type: %lu\n", g_test_type);

    switch(g_test_type) {
    case TEST_TYPE_BUG_INT:
        BUG();
        break;
    case TEST_TYPE_PANIC_INT:
        panic("test panic\n");
        break;
    case TEST_TYPE_OOPS_INT:
        create_oops();
        break;
    default:
        break;
    }
    return;
}

static void init_test_exception_timer(void)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 15, 0)
    setup_timer(&g_test_exception_timer, test_exception_timer_cb, 0);
#else
    timer_setup(&g_test_exception_timer, test_exception_timer_cb, 0);
#endif
    g_test_exception_timer.expires = jiffies + 1 * HZ;

    add_timer(&g_test_exception_timer);
    return;
}

static int help_print(struct seq_file *m, void *v)
{
    seq_printf(m, "support test list:\n"                \
        "usage: echo xxx > /proc/test_exception\n"      \
        "bug:       test bug   in process context\n"    \
        "panic:     test panic in process context\n"    \
        "oops:      test oops  in process context\n"    \
        "bug_int:   test bug   in interrupt context\n"  \
        "panic_int: test panic in interrupt context\n"  \
        "oops_int:  test oops  in interrupt context\n");

    return 0;
}

static int reg_print_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, help_print, NULL);
}

/* echo xxxx > /proc/test_exception */
static ssize_t exception_trigger_func(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    char para[USR_PARA_MAX_SIZE];
    int rv;

    DEBUG_INFO("count: %zx \n", count);
        
    if ((count < 1) || (count > (USR_PARA_MAX_SIZE - 1))) {
        return -EINVAL;
    }

    memset(para, 0, USR_PARA_MAX_SIZE);

    if (copy_from_user(para, buffer, count)) {
        return -EFAULT;
    }

    DEBUG_INFO("para: %s\n", para);

    if ((rv = strncmp(para, "bug_int", 7)) == 0) {
        g_test_type = TEST_TYPE_BUG_INT;
    } else if ((rv = strncmp(para, "bug", 3)) == 0) {
        g_test_type = TEST_TYPE_BUG;
    } else if ((rv = strncmp(para, "panic_int", 9)) == 0) {
        g_test_type = TEST_TYPE_PANIC_INT;
    } else if ((rv = strncmp(para, "panic", 5)) == 0) {
        g_test_type = TEST_TYPE_PANIC;
    } else if ((rv = strncmp(para, "oops_int", 8)) == 0) {
        g_test_type = TEST_TYPE_OOPS_INT;
    } else if ((rv = strncmp(para, "oops", 4)) == 0) {
        g_test_type = TEST_TYPE_OOPS;
    } else {
        g_test_type = TEST_TYPE_INVAILD;
        DEBUG_INFO("g_test_type: %lu\n", g_test_type);
        return count;
    }

    printk("g_test_type: %lu\n", g_test_type);
    switch(g_test_type) {
    case TEST_TYPE_BUG:
        BUG();
        break;
    case TEST_TYPE_PANIC:
        panic("test panic\n");
        break;
    case TEST_TYPE_OOPS:
        create_oops();
        break;
    case TEST_TYPE_OOPS_INT:
    case TEST_TYPE_BUG_INT:
    case TEST_TYPE_PANIC_INT:
        init_test_exception_timer();
        break;
    default:
        break;
    }
    
    return count;
}

static const struct proc_ops test_exception_proc_fops = {
    .proc_open = reg_print_proc_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
    .proc_write   = exception_trigger_func,
};

static int __init test_exception_init(void)
{
    if (proc_create(PROC_FILE_NAME, S_IRUGO, NULL, &test_exception_proc_fops) == NULL) {
        printk(KERN_ERR "create proc file: %s fail\n", PROC_FILE_NAME);
        return -1;
    }

    return 0;
}

static void __exit test_exception_exit(void)
{
    remove_proc_entry(PROC_FILE_NAME, NULL);
    return;
}

MODULE_AUTHOR("xxx");
MODULE_DESCRIPTION("exception test driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");

module_init(test_exception_init);
module_exit(test_exception_exit);

