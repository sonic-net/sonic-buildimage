#ifndef __JBISTUB_H__
#define __JBISTUB_H__

#include <linux/types.h>
#include <linux/ctype.h>
#include <linux/kernel.h>
#include <linux/gfp.h>
#include <linux/mm.h>
#include <linux/delay.h>
#include <linux/slab.h>

#ifdef CONFIG_64BIT
typedef s64 addr_t;
#else
typedef s32 addr_t;
#endif
/* typedef long addr_t; */

/* #define USE_STATIC_MEMORY   100 */
/* #define MEM_TRACKER */

/* #define O_RDWR  1 */

#define stdout  (1)
#define stderr  (2)

#define puts    printk
#define printf  printk

#define fprintf(std, fmt, arg...)   \
    do {                            \
        printf(fmt, ##arg);         \
    } while (0)

#define DEBUG_NONE      0
#define DEBUG_ERR       1
#define DEBUG_DETAIL    2
#define DEBUG_NOISY     3
#define DEBUG_MM        4

#define jbi_dbg(level, fmt, arg...)         \
        do {                                \
            if (level <= jbi_debug_level) { \
                printf(fmt, ##arg);         \
            }                               \
        } while (0)

extern int jbi_debug_level;

static inline int open(char *path, int flag)
{
    return 0;
}

static inline int close(int fd)
{
    return 0;
}

static inline int read(int fd, char *buf, int count)
{
    return 0;
}

static inline int write(int fd, char *buf, int count)
{
    return 0;
}

static inline int fflush(int fd)
{
    return 0;
}

static inline int clock(void)
{
    return 0;
}

static inline int atoi(const char *nptr)
{
    return (int) simple_strtol(nptr, (char **) NULL, 10);
}

static inline void *malloc(size_t size)
{
    return kmalloc(size, GFP_KERNEL);
}

static inline void free(void *ptr)
{
    kfree(ptr);
}

#endif /* __JBISTUB_H__ */
