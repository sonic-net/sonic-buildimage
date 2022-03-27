#ifndef __CLX_CLX_TYPES_H__
#define __CLX_CLX_TYPES_H__

#ifdef __KERNEL__
#include <linux/types.h>
#else
#include <stdint.h>
#endif

typedef uint32_t clx_port_t;
typedef uint8_t clx_mac_t[6];

#if defined(CLX_EN_HOST_64_BIT_BIG_ENDIAN) || defined(CLX_EN_HOST_64_BIT_LITTLE_ENDIAN) || \
    defined(CLX_EN_64BIT_ADDR)
typedef unsigned long long int clx_addr_t;
typedef unsigned long long int clx_huge_t;

#define clx_addr_64_hi(__addr__)  ((uint32_t)((__addr__) >> 32))
#define clx_addr_64_low(__addr__) ((uint32_t)((__addr__) & 0xFFFFFFFF))
#define clx_addr_32_to_64(__hi32__, __low32__) \
    (((unsigned long long int)(__low32__)) | (((unsigned long long int)(__hi32__)) << 32))
#else
typedef unsigned int clx_addr_t;
typedef unsigned int clx_huge_t;

#define clx_addr_64_hi(__addr__)               (0)
#define clx_addr_64_low(__addr__)              (__addr__)
#define clx_addr_32_to_64(__hi32__, __low32__) (__low32__)
#endif

// NOTICE: This is a enum for ioctrl error code same as clx_error_no_t
//        it should be updated after ioctrl is updated
typedef enum clx_ioctl_error_no_e {
    CLX_IOCTL_E_OK = 0,          /* Ok and no error */
    CLX_IOCTL_E_BAD_PARAMETER,   /* Parameter is wrong */
    CLX_IOCTL_E_NO_MEMORY,       /* No memory is available */
    CLX_IOCTL_E_TABLE_FULL,      /* Table is full */
    CLX_IOCTL_E_ENTRY_NOT_FOUND, /* Entry is not found */
    CLX_IOCTL_E_ENTRY_EXISTS,    /* Entry already exists */
    CLX_IOCTL_E_NOT_SUPPORT,     /* Feature is not supported */
    CLX_IOCTL_E_ALREADY_INITED,  /* Module is reinitialized */
    CLX_IOCTL_E_NOT_INITED,      /* Module is not initialized */
    CLX_IOCTL_E_OTHERS,          /* Other errors */
    CLX_IOCTL_E_ENTRY_IN_USE,    /* Entry is in use */
    CLX_IOCTL_E_TIMEOUT,         /* Time out error */
    CLX_IOCTL_E_OP_INVALID,      /* Operation is invalid */
    CLX_IOCTL_E_OP_STOPPED,      /* Operation is stopped by user callback */
    CLX_IOCTL_E_OP_INCOMPLETE,   /* Operation is incomplete */
    CLX_IOCTL_E_TRY_AGAIN,       /* Opertion not exec, try again */
    CLX_IOCTL_E_BAD_CASE,        /* Switch case err */
    CLX_IOCTL_E_LAST
} clx_ioctl_error_no_t;

#endif // __CLX_CLX_TYPES_H__