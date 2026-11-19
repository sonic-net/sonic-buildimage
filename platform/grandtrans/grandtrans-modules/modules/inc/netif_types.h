#ifndef _NETIF_TYPES_H_
#define _NETIF_TYPES_H_

#include <linux/types.h>

typedef uint8_t   UI8_T;
typedef uint16_t  UI16_T;
typedef uint32_t  UI32_T;
typedef uint64_t  UI64_T;
typedef int8_t    I8_T;
typedef int16_t   I16_T;
typedef int32_t   I32_T;
typedef int64_t   I64_T;
typedef char      C8_T;

typedef int       GT_ERROR_NO_T;
#define GT_E_OK           0
#define GT_E_OTHERS       1
#define GT_E_OP_INVALID   2

typedef uint8_t   gt_mac_t[6];

typedef int       gt_ioctl_error_no_t;
#define gt_IOCTL_E_ENTRY_NOT_FOUND  1

typedef unsigned long  GT_THREAD_ID_T;
typedef unsigned long  GT_SEMAPHORE_ID_T;
typedef unsigned long  GT_ISRLOCK_ID_T;
typedef unsigned long  GT_IRQ_FLAGS_T;
typedef unsigned long  GT_HUGE_T;
typedef uint64_t       GT_TIME_T;

typedef enum gt_ioctl_type_s{
    GT_IOCTL_INIT_DEV = 0,
    NETIF_IOCTL_TYPE_CREATE_INTF = 100,
    NETIF_IOCTL_TYPE_DESTROY_INTF,
    NETIF_IOCTL_TYPE_GET_INTF,
    NETIF_IOCTL_TYPE_SET_INTF,
    NETIF_IOCTL_TYPE_CREATE_PROFILE,
    NETIF_IOCTL_TYPE_DESTROY_PROFILE,
    NETIF_IOCTL_TYPE_GET_PROFILE,
    NETIF_IOCTL_TYPE_GET_INTF_CNT,
    NETIF_IOCTL_TYPE_CLEAR_INTF_CNT,
    NETIF_IOCTL_TYPE_LAST
} gt_ioctl_type_t;

typedef union {
    uint32_t value;
    struct {
        uint32_t chip : 7;
        uint32_t type : 10;
        uint32_t rsvd : 15;
    } field;
} gt_ioctl_cmd_t;

#include <linux/stddef.h> /* for bool, true, false */
typedef bool BOOL;
#ifndef TRUE
#define TRUE  true
#endif
#ifndef FALSE
#define FALSE false
#endif

#endif
