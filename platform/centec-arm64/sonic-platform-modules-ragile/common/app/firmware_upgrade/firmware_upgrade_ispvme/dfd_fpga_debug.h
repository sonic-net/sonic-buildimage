#ifndef _DFD_FPGA_DEBUG_H_
#define _DFD_FPGA_DEBUG_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define DFD_DEBUG_FILE                 "/sbin/.dfd_debug_flag"

#define DFD_DEBUG_SET_NO_WARN          "0x1"
#define DFD_DEBUG_SET_NO_VBOSE         "0x3"
#define DFD_DEBUG_SET_NO_FLOCK_VBOSE   "0x7"
#define DFD_DEBUG_SET_ALL              "0xf"
#define DFD_DEBUG_SET_DBG              "0xd"
#define DFD_DEBUG_SET_FLOCK            "0xe"
#define mem_clear(data, size) memset((data), 0, (size))

#define DFD_DEBUG_CHECK(type)      (g_dfd_fpga_debug & (1U << (type)))

#define DFD_ERROR(fmt, args...) do {                     \
    if (DFD_DEBUG_CHECK(DFD_DBG_ERR)) {               \
        printf("[%s-%s]:<File:%s, Func:%s, Line:%d>\n" fmt, "DFD", "err", \
            __FILE__, __FUNCTION__, __LINE__, ##args);  \
    }                                                   \
} while (0)

#define DFD_WARN(fmt, args...) do {                     \
    if (DFD_DEBUG_CHECK(DFD_DBG_WARN)) {               \
        printf("[%s-%s]:<File:%s, Func:%s, Line:%d>\n" fmt, "DFD", "warn", \
            __FILE__, __FUNCTION__, __LINE__, ##args);  \
    }                                                   \
} while (0)

#define DFD_VERBOS(fmt, args...) do {                     \
    if (DFD_DEBUG_CHECK(DFD_DBG_VBOSE)) {               \
        printf("[%s-%s]:<File:%s, Func:%s, Line:%d>\n" fmt, "DFD", "vbose", \
            __FILE__, __FUNCTION__, __LINE__, ##args);  \
    }                                                   \
} while (0)

#define DFD_FLOCK_VERBOS(fmt, args...) do {                     \
    if (DFD_DEBUG_CHECK(DFD_FLOCK_DBG_VBOSE)) {               \
        printf("[%s-%s]:<File:%s, Func:%s, Line:%d>\n" fmt, "DFD", "flock_vbose", \
            __FILE__, __FUNCTION__, __LINE__, ##args);  \
    }                                                   \
} while (0)

#define DFD_DBG(fmt, args...) do {                     \
        if (DFD_DEBUG_CHECK(DFD_DBG_DBG)) {               \
            printf("" fmt,\
                ##args);  \
        }                                                   \
    } while (0)
/* define the type of debugging information */
typedef enum {
    DFD_DBG_ERR,
    DFD_DBG_WARN,
    DFD_DBG_VBOSE,
    DFD_FLOCK_DBG_VBOSE,
    DFD_DBG_DBG,
    DFD_DBG_CNT
} DFD_DEBUG_TYPE_E;

extern int g_dfd_fpga_debug;

int dfd_fpga_debug_set(int type, int enable);
void dfd_fpga_open_debug(int val);
void dfd_fpga_debug_init(void);

#endif