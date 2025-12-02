/* wb_bsp_i2c_debug.h */
#ifndef __WB_BSP_I2C_DEBUG_H__
#define __WB_BSP_I2C_DEBUG_H__

#include <linux/i2c.h>
#include <linux/sysfs.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include "wb_bsp_kernel_debug.h"

/* 0x12caddb9 looks a lot like "i2c ad dbg". */
#define MAGIC_NUM   (0x12caddb9)

/*
 * The struct i2c_adapter_debug must be the first member 
 *  of the private structure of the I2C controller. 
 */
struct i2c_adapter_debug {
    int magic_num;
    int debug_level;
};

/**
 * create_adapter_dbg - Create debug info entry for the specified adapter
 * @adapter: specified i2c_adapter
 *
 * Returns 0 on success, negative error code on failure
 */
static inline int create_adapter_dbg(struct i2c_adapter *adapter)
{
    struct i2c_adapter_debug *dbg;

    if (adapter == NULL) {
        return -EINVAL;
    }

    dbg = (struct i2c_adapter_debug *)(i2c_get_adapdata(adapter));
    if (dbg == NULL) {
        return -EINVAL;
    }

    dbg->magic_num = MAGIC_NUM;
    dbg->debug_level = 0;

    return 0;
}

/**
 * destroy_adapter_dbg - Remove debug info entry for the specified adapter
 * @adapter: specified i2c_adapter
 */
static inline void destroy_adapter_dbg(struct i2c_adapter *adapter)
{
    struct i2c_adapter_debug *dbg;

    if (adapter == NULL) {
        return;
    }

    dbg = (struct i2c_adapter_debug *)(i2c_get_adapdata(adapter));
    if (dbg == NULL) {
        return;
    }
    dbg->magic_num = 0;
    dbg->debug_level = 0;

    return;
}

/**
 * get_adapter_dbg - Get debug info pointer for the specified adapter
 * @adapter: specified i2c_adapter
 *
 * Returns pointer to debug info, or NULL if not found
 */
static inline struct i2c_adapter_debug *get_adapter_dbg(struct i2c_adapter *adapter)
{
    struct i2c_adapter_debug *dbg;

    if (adapter == NULL) {
        return NULL;
    }

    dbg = (struct i2c_adapter_debug *)(i2c_get_adapdata(adapter));
    if ((dbg != NULL) && (dbg->magic_num != MAGIC_NUM)) {
        return NULL;
    }

    return dbg;
}

/* sysfs show function, displays debug enable status */
static ssize_t debug_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    struct i2c_adapter *adap;
    struct i2c_adapter_debug *dbg;
    int offset;

    adap = container_of(kobj, struct i2c_adapter, dev.kobj);
    dbg = get_adapter_dbg(adap);

    if (dbg == NULL || dbg->magic_num != MAGIC_NUM || dbg->debug_level == 0) {
        return scnprintf(buf, PAGE_SIZE, "Current debug status: %s\n", "disabled");
    }

    offset = scnprintf(buf, PAGE_SIZE, "Current debug status: %s\n", "enabled");
    if (dbg->debug_level & DEBUG_ERR_LEVEL) {
        offset += scnprintf(buf + offset, PAGE_SIZE - offset, "\terror\n");
    }
    if (dbg->debug_level & DEBUG_WARN_LEVEL) {
        offset += scnprintf(buf + offset, PAGE_SIZE - offset, "\twarn\n");
    }
    if (dbg->debug_level & DEBUG_INFO_LEVEL) {
        offset += scnprintf(buf + offset, PAGE_SIZE - offset, "\tinfo\n");
    }
    if (dbg->debug_level & DEBUG_VER_LEVEL) {
        offset += scnprintf(buf + offset, PAGE_SIZE - offset, "\tverbose\n");
    }

    return offset;
}

/* sysfs store function, sets debug enable status */
static ssize_t debug_store(struct kobject *kobj, struct kobj_attribute *attr,
                           const char *buf, size_t count)
{
    struct i2c_adapter *adap;
    struct i2c_adapter_debug *dbg;

    adap = container_of(kobj, struct i2c_adapter, dev.kobj);
    dbg = get_adapter_dbg(adap);
    if (dbg == NULL) {
        return -EINVAL;
    }

    if (sysfs_streq(buf, "0")) {
        dbg->debug_level = 0;
    } else if (sysfs_streq(buf, "1")) {
        dbg->debug_level |= DEBUG_ERR_LEVEL;
    } else if (sysfs_streq(buf, "2")) {
        dbg->debug_level |= DEBUG_WARN_LEVEL;
    } else if (sysfs_streq(buf, "4")) {
        dbg->debug_level |= DEBUG_INFO_LEVEL;
    } else if (sysfs_streq(buf, "8")) {
        dbg->debug_level |= DEBUG_VER_LEVEL;
    } else {
        pr_info("0: clear, 1: error, 2: warn, 4: info, 8: verbose.\n");
        return -EINVAL;
    }

    pr_info("i2c adapter %d debug %d\n", adap->nr, dbg->debug_level);

    return count;
}

static struct kobj_attribute debug_attribute =
    __ATTR(debug, 0664, debug_show, debug_store);

/**
 * i2c_adapter_debug_init - Initialize debug info and create sysfs file for adapter
 * @adap: specified i2c_adapter
 *
 * Returns 0 on success, negative error code on failure
 */
static inline int i2c_adapter_debug_init(struct i2c_adapter *adap)
{
    int ret;

    ret = create_adapter_dbg(adap);
    if (ret) {
        return ret;
    }

    ret = sysfs_create_file(&adap->dev.kobj, &debug_attribute.attr);
    if (ret) {
        /* On failure, rollback created debug info */
        destroy_adapter_dbg(adap);
    }
    return ret;
}

/**
 * I2C_ADAPTER_DEBUG_INIT - Debug initialization macro (compile-time check + runtime init)
 * @dev_type: Structure type (e.g., struct dw_i2c_dev)
 * @dev_ptr: Pointer to the structure instance (e.g., dev)
 * @dbg_member: Name of the debug member in the structure (e.g., i2c_ada_dbg)
 *
 * Description:
 * 1. Compile-time check: Ensures the debug member is the first member of the structure
 *    (offset must be 0; fails compilation if not).
 * 2. Runtime initialization: Calls the debug initialization function to bind I2C adapter
 *    with debug functionality (sysfs nodes, debug level management, etc.).
 */
#define I2C_ADAPTER_DEBUG_INIT(adapter, dev_type, dbg_member) do { \
    /* Compile-time check: Debug struct must be the first member (offset 0) */ \
    BUILD_BUG_ON(offsetof(dev_type, dbg_member) != 0); \
    /* Runtime initialization: Initialize debug features for the I2C adapter */ \
    (void)i2c_adapter_debug_init(adapter); \
} while (0)

/**
 * i2c_adapter_debug_exit - Cleanup debug info and remove sysfs file for adapter
 * @adap: specified i2c_adapter
 */
static inline void i2c_adapter_debug_exit(struct i2c_adapter *adap)
{
    struct i2c_adapter_debug *dbg;

    dbg = get_adapter_dbg(adap);
    if (dbg == NULL) {
        return;
    }

    sysfs_remove_file(&adap->dev.kobj, &debug_attribute.attr);
    destroy_adapter_dbg(adap);
}

static inline bool i2c_adapter_debug_enabled(struct i2c_adapter *adap, int level)
{
    struct i2c_adapter_debug *dbg;

    if (adap == NULL) {
        return false;
    }

    dbg = get_adapter_dbg(adap);
    if (dbg == NULL) {
        return false;
    }

    return (dbg->debug_level & level) != 0;
}

static inline int i2c_adapter_get_nr(struct i2c_adapter *adap)
{
    return (adap == NULL) ? -1 : adap->nr;
}

#define i2c_pr_fmt(fmt, nr) "[func:%s line:%d][i2c-%d]" fmt, __func__, __LINE__, (nr)

/* Debug print macros, depend on external variable debug and DEBUG_*_LEVEL macros */
#define DEBUG_ERROR_I2C_ADAPTER(adapter, fmt, ...) \
    do { \
        if ((debug & DEBUG_ERR_LEVEL) || i2c_adapter_debug_enabled(adapter, DEBUG_ERR_LEVEL)) { \
            printk(KERN_ERR "[ERR]"i2c_pr_fmt(fmt, i2c_adapter_get_nr(adapter)), ##__VA_ARGS__); \
        } else { \
            pr_debug(fmt, ##__VA_ARGS__); \
        } \
    } while(0)

#define DEBUG_WARN_I2C_ADAPTER(adapter, fmt, ...) \
    do { \
        if ((debug & DEBUG_WARN_LEVEL) || i2c_adapter_debug_enabled(adapter, DEBUG_WARN_LEVEL)) { \
            printk(KERN_WARNING "[WARN]"i2c_pr_fmt(fmt, i2c_adapter_get_nr(adapter)), ##__VA_ARGS__); \
        } else { \
            pr_debug(fmt, ##__VA_ARGS__); \
        } \
    } while(0)

#define DEBUG_INFO_I2C_ADAPTER(adapter, fmt, ...) \
    do { \
        if ((debug & DEBUG_INFO_LEVEL) || i2c_adapter_debug_enabled(adapter, DEBUG_INFO_LEVEL)) { \
            printk(KERN_INFO "[INFO]"i2c_pr_fmt(fmt, i2c_adapter_get_nr(adapter)), ##__VA_ARGS__); \
        } else { \
            pr_debug(fmt, ##__VA_ARGS__); \
        } \
    } while(0)

#define DEBUG_VERBOSE_I2C_ADAPTER(adapter, fmt, ...) \
    do { \
        if ((debug & DEBUG_VER_LEVEL) || i2c_adapter_debug_enabled(adapter, DEBUG_VER_LEVEL)) { \
            printk(KERN_DEBUG "[VER]"i2c_pr_fmt(fmt, i2c_adapter_get_nr(adapter)), ##__VA_ARGS__); \
        } else { \
            pr_debug(fmt, ##__VA_ARGS__); \
        } \
    } while(0)

#endif  /* __WB_BSP_I2C_DEBUG_H__ */
