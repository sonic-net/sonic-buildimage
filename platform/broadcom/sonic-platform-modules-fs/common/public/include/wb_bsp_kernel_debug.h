/* kernel_debug.h */
#ifndef __WB_BSP_KERNEL_DEBUG_H__
#define __WB_BSP_KERNEL_DEBUG_H__

/********************************** start ***********************************/
/**
 * Module input parameters control debug printing.
 * Dynamic debug printing is used by default, controlled by debugfs
 */
#define DEBUG_ERR_LEVEL                     (0x1)
#define DEBUG_WARN_LEVEL                    (0x2)
#define DEBUG_INFO_LEVEL                    (0x4)
#define DEBUG_VER_LEVEL                     (0x8)
/* This macro definition is specifically designed for printing logic device driver data and is not intended for any other debugging purposes. */
#define DEBUG_DUMP_DATA_LEVEL               (0x8000)

#define DEBUG_ERROR(fmt, args...)                                                             \
    do {                                                                                      \
        if (debug & DEBUG_ERR_LEVEL) {                                                        \
            printk(KERN_ERR "[ERR][func:%s line:%d]  "fmt, __func__, __LINE__, ## args);      \
        } else {                                                                              \
            pr_debug(fmt, ## args);                                                           \
        }                                                                                     \
    } while(0)

#define DEBUG_WARN(fmt, args...)                                                              \
    do {                                                                                      \
        if (debug & DEBUG_WARN_LEVEL) {                                                       \
            printk(KERN_WARNING "[WARN][func:%s line:%d]  "fmt, __func__, __LINE__, ## args); \
        } else {                                                                              \
            pr_debug(fmt, ## args);                                                           \
        }                                                                                     \
    } while(0)

#define DEBUG_INFO(fmt, args...)                                                              \
    do {                                                                                      \
        if (debug & DEBUG_INFO_LEVEL) {                                                       \
            printk(KERN_INFO "[INFO][func:%s line:%d]  "fmt, __func__, __LINE__, ## args);    \
        } else {                                                                              \
            pr_debug(fmt, ## args);                                                           \
        }                                                                                     \
    } while(0)

#define DEBUG_VERBOSE(fmt, args...)                                                           \
    do {                                                                                      \
        if (debug & DEBUG_VER_LEVEL) {                                                        \
            printk(KERN_DEBUG "[VER][func:%s line:%d]  "fmt, __func__, __LINE__, ## args);    \
        } else {                                                                              \
            pr_debug(fmt, ## args);                                                           \
        }                                                                                     \
    } while(0)
/********************************** end ***********************************/

#endif  /* #ifndef __WB_BSP_KERNEL_DEBUG_H__ */
