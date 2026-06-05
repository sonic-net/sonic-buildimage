/*
 * $Id: phymod_types.h,v 1.1.2.2 Broadcom SDK $
 * $Copyright: (c) 2020 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.$
 *
 * Basic type definitions.
 */

#ifndef __plp_aperta2_PHYMOD_TYPES_H__
#define __plp_aperta2_PHYMOD_TYPES_H__

#include <phymod/phymod_config.h>

#ifndef FUNCTION_NAME
#define FUNCTION_NAME() (__func__)
#endif

#if PHYMOD_CONFIG_DEFINE_UINT8_T == 1
typedef PHYMOD_CONFIG_TYPE_UINT8_T uint8_t; 
#endif

#if PHYMOD_CONFIG_DEFINE_UINT16_T == 1
typedef PHYMOD_CONFIG_TYPE_UINT16_T uint16_t; 
#endif

#if PHYMOD_CONFIG_DEFINE_UINT32_T == 1
typedef PHYMOD_CONFIG_TYPE_UINT32_T uint32_t; 
#endif

#if PHYMOD_CONFIG_DEFINE_UINT64_T == 1
#ifndef WIN32
typedef PHYMOD_CONFIG_TYPE_UINT64_T uint64_t;
#endif
#endif

#if PHYMOD_CONFIG_DEFINE_INT8_T == 1
#ifndef SIM_VAL
typedef PHYMOD_CONFIG_TYPE_INT8_T int8_t; 
#endif
#endif

#if PHYMOD_CONFIG_DEFINE_INT16_T == 1
#ifndef SIM_VAL
typedef PHYMOD_CONFIG_TYPE_INT16_T int16_t; 
#endif
#endif

#if PHYMOD_CONFIG_DEFINE_INT32_T == 1
#ifndef SIM_VAL
typedef PHYMOD_CONFIG_TYPE_INT32_T int32_t; 
#endif
#endif

#if PHYMOD_CONFIG_DEFINE_SIZE_T == 1
#ifndef SIM_VAL
typedef PHYMOD_CONFIG_TYPE_SIZE_T size_t; 
#endif
#endif

#if PHYMOD_CONFIG_DEFINE_PRIx32 == 1
#define PRIx32 PHYMOD_CONFIG_MACRO_PRIx32
#endif

#if PHYMOD_CONFIG_DEFINE_PRIu32 == 1
#define PRIu32 PHYMOD_CONFIG_MACRO_PRIu32
#endif

#if PHYMOD_CONFIG_DEFINE_PRIu64 == 1
#define PRIu64 PHYMOD_CONFIG_MACRO_PRIu64
#endif

#if PHYMOD_CONFIG_DEFINE_ERROR_CODES
typedef enum {
    PHYMOD_E_NONE       = 0,
    PHYMOD_E_INTERNAL   = -1,
    PHYMOD_E_MEMORY     = -2,
    PHYMOD_E_IO         = -3,
    PHYMOD_E_PARAM      = -4,
    PHYMOD_E_CORE       = -5,
    PHYMOD_E_PHY        = -6,
    PHYMOD_E_BUSY       = -7,
    PHYMOD_E_FAIL       = -8,
    PHYMOD_E_TIMEOUT    = -9,
    PHYMOD_E_RESOURCE   = -10,
    PHYMOD_E_CONFIG     = -11,
    PHYMOD_E_UNAVAIL    = -12,
    PHYMOD_E_INIT       = -13,
    PHYMOD_E_LIMIT      = -14           /* Must come last */
} plp_aperta2_phymod_error_t;
#endif

#ifndef NULL
#define NULL (void*)0
#endif

#ifndef STATIC
#define STATIC static
#endif

#ifndef VOLATILE
#define VOLATILE volatile
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef COUNTOF
#define COUNTOF(ary) ((int) (sizeof(ary) / sizeof((ary)[0])))
#endif

#ifndef PTR2INT
#define PTR2INT(_p) ((int)((long)(_p)))
#endif

#ifndef INT2PTR
#define INT2PTR(_i) ((void *)((long)(_i)))
#endif

#define LSHIFT32(_val, _cnt) ((uint32_t)(_val) << (_cnt))

#ifndef COMPILER_REFERENCE
#define COMPILER_REFERENCE(_a) ((void)(_a))
#endif

#ifndef BYTES2BITS
#define BYTES2BITS(x) ((x) * 8)
#endif

#ifndef BYTES2WORDS
#define BYTES2WORDS(x) (((x) + 3) / 4)
#endif

#ifndef WORDS2BITS
#define WORDS2BITS(x) ((x) * 32)
#endif

#ifndef WORDS2BYTES
#define WORDS2BYTES(x) ((x) * 4)
#endif


/* These must be moved */
#ifndef __F_MASK
#define __F_MASK(w) \
        (((uint32_t)1 << w) - 1)
#endif

#ifndef __F_GET
#define __F_GET(d,o,w) \
        (((d) >> o) & __F_MASK(w))
#endif

#ifndef __F_SET
#define __F_SET(d,o,w,v) \
        (d = ((d & ~(__F_MASK(w) << o)) | (((v) & __F_MASK(w)) << o)))
#endif

#ifndef __F_ENCODE

/* Encode a value of a given width at a given offset. Performs compile-time error checking on the value */
/* To ensure it fits within the given width */
#define __F_ENCODE(v,o,w) \
        ( ((v & __F_MASK(w)) == v) ? /* Value fits in width */ ( (uint32_t)(v) << o ) : /* Value does not fit -- compile time error */ 1 << 99)

#endif

typedef uint32_t plp_aperta2_phy_id_t;

typedef uint32_t plp_aperta2_core_id_t;


#endif /* __plp_aperta2_PHYMOD_TYPES_H__ */
