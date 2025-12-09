/*! \file bcmdrd_types.h
 *
 * Basic DRD types, which may be used outside the DRD as well.
 *
 */
/*
 *
 * Copyright 2018-2025 Broadcom. All rights reserved.
 * The term 'Broadcom' refers to Broadcom Inc. and/or its subsidiaries.
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License 
 * version 2 as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * A copy of the GNU General Public License version 2 (GPLv2) can
 * be found in the LICENSES folder.
 */

#ifndef BCMDRD_TYPES_H
#define BCMDRD_TYPES_H

#include <bcmdrd_config.h>

#include <sal/sal_types.h>
#include <sal/sal_libc.h>

#include <shr/shr_types.h>

/*! 16-bit-safe left shift */
#define LSHIFT32(_val, _cnt) ((uint32_t)(_val) << (_cnt))

/*! 32-bit-safe left shift */
#define LSHIFT64(_val, _cnt) ((uint64_t)(_val) << (_cnt))



#ifndef F32_MASK
/*! Create a bit mask of w bits as a 32-bit word. */
#define F32_MASK(w) \
    ((((w) > 31) ? 0 : ((uint32_t)1 << (w))) - 1)
#endif

#ifndef F64_MASK
/*! Create a bit mask of w bits as a 64-bit dword. */
#define F64_MASK(w) \
    ((((w) > 63) ? 0 : ((uint64_t)1 << (w))) - 1)
#endif

#ifndef F32_GET
/*! Extract a field of w bits at offset o from a 32-bit word d. */
#define F32_GET(d,o,w) \
        (((d) >> o) & F32_MASK(w))
#endif

#ifndef F64_GET
/*! Extract a field of w bits at offset o from a 64-bit word d. */
#define F64_GET(d,o,w) \
        (((d) >> o) & F64_MASK(w))
#endif

#ifndef F32_SET
/*! Set a field of w bits at offset o in a 32-bit word d. */
#define F32_SET(d,o,w,v) \
        (d = ((d & ~(F32_MASK(w) << o)) | (((v) & F32_MASK(w)) << o)))
#endif

#ifndef F64_SET
/*! Set a field of w bits at offset o in a 64-bit word d. */
#define F64_SET(d,o,w,v) \
        (d = ((d & ~(F64_MASK(w) << o)) | (((v) & F64_MASK(w)) << o)))
#endif

/*! Optionally force an error in compiler pre-processor. */
#if BCMDRD_CONFIG_INCLUDE_FIELD_CHECKS
#define BCMDRD_COMPILER_ERROR (1 << 99)
#else
#define BCMDRD_COMPILER_ERROR 0
#endif

#ifndef F32_ENCODE
/*!
 * Encode a value of a given width at a given offset. Optionally
 * performs compile-time error checking on the value to ensure it fits
 * within the given width.
 */
#define F32_ENCODE(v,o,w) \
        ( ((v & F32_MASK(w)) == v) ? \
          /* Value fits in width */ ( (uint32_t)(v) << o ) : \
          /* Value does not fit */ BCMDRD_COMPILER_ERROR)

#endif

#ifndef F64_ENCODE
/*!
 * Encode a value of a given width at a given offset. Performs
 * compile-time error checking on the value to ensure it fits within
 * the given width.
 */
#define F64_ENCODE(v,o,w) \
        ( ((v & F64_MASK(w)) == v) ? \
          /* Value fits in width */ ( (uint64_t)(v) << o ) : \
          /* Value does not fit */ BCMDRD_COMPILER_ERROR)

#endif

/*! Words in port bit maps */
#define BCMDRD_PBMP_WORD_MAX    (((BCMDRD_CONFIG_MAX_PORTS - 1) >> 5) + 1)

/*!
 * Bitmap of ports of a particular type or properties.
 */
typedef struct bcmdrd_pbmp_s {
    /*! Word array. */
    uint32_t w[BCMDRD_PBMP_WORD_MAX];
} bcmdrd_pbmp_t;

/* Port bitmap helper functions */

/*!
 * \brief Check if port bitmap is empty.
 *
 * Check that no bits are set in a port bitmap of type \ref
 * bcmdrd_pbmp_t.
 *
 * \param [in] pbmp Port bitmap.
 *
 * \retval true Port bitmap is empty.
 * \retval false Port bitmap is not empty.
 */
extern int
bcmdrd_pbmp_is_null(const bcmdrd_pbmp_t *pbmp);

/*!
 * \brief Parse a port list string into a port bitmap.
 *
 * The port list string may contain commas to separate port numbers
 * and hyphens to indicate port ranges.
 *
 * Examples: "2" "2,5" "2,5,7-13,43"
 *
 * \param [in] str String to be parsed.
 * \param [out] pbmp Port bitmap.
 *
 * \retval 0 No errors.
 * \retval -1 Fail to parse the string to a port bitmap.
 */
extern int
bcmdrd_pbmp_parse(const char *str, bcmdrd_pbmp_t *pbmp);

/*!
 * \brief Get bitmap word index for a given port.
 *
 * A port bitmap is an array of data words, and this macro will return
 * the index of the data word associated with a given port number.
 *
 * No range check is performed on the port number.
 *
 * \param [in] _pbmp Port bitmap.
 * \param [in] _port Port number to check.
 */
#define BCMDRD_PBMP_WORD(_pbmp, _port)          \
    (&(_pbmp))->w[(_port) >> 5]

/*!
 * \brief Check if a port is member of a port bitmap.
 *
 * Check if a port is member of a port bitmap of type \ref
 * bcmdrd_pbmp_t.
 *
 * No range check is performed on the port number.
 *
 * \param [in] _pbmp Port bitmap.
 * \param [in] _port Port number to check.
 */
#define BCMDRD_PBMP_MEMBER(_pbmp, _port) \
     (BCMDRD_PBMP_WORD(_pbmp, _port) & LSHIFT32(1, (_port) & 0x1f))

/*!
 * \brief Iterate over a port bitmap.
 *
 * Iterate over a port bitmap of type \ref bcmdrd_pbmp_t and execute
 * the subsequent statement for all bits set in the port bitmap.
 *
 * \param [in] _pbmp Port bitmap.
 * \param [out] _port Port iterator variable.
 */
#define BCMDRD_PBMP_ITER(_pbmp, _port)                          \
    for (_port = 0; _port < BCMDRD_CONFIG_MAX_PORTS; _port++)   \
        if (BCMDRD_PBMP_WORD(_pbmp, _port) == 0)                \
            _port += 31;                                        \
        else if (BCMDRD_PBMP_MEMBER(_pbmp, _port))

/*!
 * \brief Iterate over a port bitmap with maximum.
 *
 * Iterate over a port bitmap of type \ref bcmdrd_pbmp_t and execute
 * the subsequent statement for all bits set in the port bitmap.
 *
 * The iteration will be terminated if the iterator variable reaches
 * the value of \c _port_max. This macro is mainly intended to prevent
 * false errors from static analysis tools like Coverity.
 *
 * \param [in] _pbmp Port bitmap.
 * \param [in] _port_max Maximum number of ports.
 * \param [out] _port Port iterator variable.
 */
#define BCMDRD_PBMP_MAX_ITER(_pbmp, _port_max, _port)           \
    for (_port = 0;                                             \
         _port < _port_max && _port < BCMDRD_CONFIG_MAX_PORTS;  \
         _port++)                                               \
        if (BCMDRD_PBMP_WORD(_pbmp, _port) == 0)                \
            _port += 31;                                        \
        else if (BCMDRD_PBMP_MEMBER(_pbmp, _port))

/*!
 * \brief Add a port to a port bitmap.
 *
 * Add a port to a port bitmap of type \ref bcmdrd_pbmp_t.
 *
 * No range check is performed on the port number.
 *
 * \param [in] _pbmp Port bitmap.
 * \param [in] _port Port number to add.
 */
#define BCMDRD_PBMP_PORT_ADD(_pbmp, _port) \
     (BCMDRD_PBMP_WORD(_pbmp, _port) |= LSHIFT32(1, (_port) & 0x1f))

/*!
 * \brief Remove a port from a port bitmap.
 *
 * Remove a port from a port bitmap of type \ref bcmdrd_pbmp_t.
 *
 * No range check is performed on the port number.
 *
 * \param [in] _pbmp Port bitmap.
 * \param [in] _port Port number to remove.
 */
#define BCMDRD_PBMP_PORT_REMOVE(_pbmp, _port) \
     (BCMDRD_PBMP_WORD(_pbmp, _port) &= ~(LSHIFT32(1, (_port) & 0x1f)))

/*!
 * \brief Clear a port bitmap.
 *
 * Clear a port bitmap of type \ref bcmdrd_pbmp_t.
 *
 * After clearing the port bitmap, it will have no members..
 *
 * \param [in] _pbmp Port bitmap.
 */
#define BCMDRD_PBMP_CLEAR(_pbmp) sal_memset(&_pbmp, 0, sizeof(bcmdrd_pbmp_t))

/*!
 * \brief Get a word from a port bitmap.
 *
 * Get a 32-bit word from a port bitmap of type \ref bcmdrd_pbmp_t.
 *
 * \param [in] _pbmp Port bitmap.
 * \param [in] _w Word number to get (first word is word 0).
 */
#define BCMDRD_PBMP_WORD_GET(_pbmp, _w)            ((&(_pbmp))->w[_w])

/*!
 * \brief Set a word in a port bitmap.
 *
 * Set a 32-bit word in a port bitmap of type \ref bcmdrd_pbmp_t.
 *
 * \param [in] _pbmp Port bitmap.
 * \param [in] _w Word number to set (first word is word 0).
 * \param [in] _val Value of word to set.
 */
#define BCMDRD_PBMP_WORD_SET(_pbmp, _w, _val)      ((&(_pbmp))->w[_w]) = (_val)

/*!
 * \brief Helper macro for port bimap operations.
 *
 * \param [in] _pbmp0 First port bitmap.
 * \param [in] _pbmp1 Second port bitmap.
 * \param [in] _op Port bitmap operator.
 */
#define BCMDRD_PBMP_BMOP(_pbmp0, _pbmp1, _op) \
    do { \
        int _w; \
        for (_w = 0; _w < BCMDRD_PBMP_WORD_MAX; _w++) { \
            BCMDRD_PBMP_WORD_GET(_pbmp0, _w) _op BCMDRD_PBMP_WORD_GET(_pbmp1, _w); \
        } \
    } while (0)

/*! Return true if port bitmap _pbmp is empty. */
#define BCMDRD_PBMP_IS_NULL(_pbmp)         (bcmdrd_pbmp_is_null(&(_pbmp)))

/*! Return true if port bitmap _pbmp is not empty. */
#define BCMDRD_PBMP_NOT_NULL(_pbmp)        (!(bcmdrd_pbmp_is_null(&(_pbmp))))

/*! Assign port bitmap src to port bitmap dst. */
#define BCMDRD_PBMP_ASSIGN(dst, src)       sal_memcpy(&(dst), &(src), sizeof(bcmdrd_pbmp_t))

/*!
 * Perform a logical AND operation between all bits of port bitmaps _pbmp0 and
 * pbmp1.
 */
#define BCMDRD_PBMP_AND(_pbmp0, _pbmp1)    BCMDRD_PBMP_BMOP(_pbmp0, _pbmp1, &=)

/*!
 * Perform a logical OR operation between all bits of port bitmaps _pbmp0 and
 * pbmp1.
 */
#define BCMDRD_PBMP_OR(_pbmp0, _pbmp1)     BCMDRD_PBMP_BMOP(_pbmp0, _pbmp1, |=)

/*!
 * Perform a logical XOR operation between all bits of port bitmaps _pbmp0 and
 * pbmp1.
 */
#define BCMDRD_PBMP_XOR(_pbmp0, _pbmp1)    BCMDRD_PBMP_BMOP(_pbmp0, _pbmp1, ^=)

/*!
 * Remove all bits in port bitmap _pbmp1 from port bitmap _pbmp0.
 */
#define BCMDRD_PBMP_REMOVE(_pbmp0, _pbmp1) BCMDRD_PBMP_BMOP(_pbmp0, _pbmp1, &= ~)

/*!
 * Assign an inversed port bitmap _pbmp1 to port bitmap _pbmp0,
 * i.e. any port which is a member of _pbmp1 will not be a member of
 * _pbmp0 and vice versa.
 */
#define BCMDRD_PBMP_NEGATE(_pbmp0, _pbmp1) BCMDRD_PBMP_BMOP(_pbmp0, _pbmp1, = ~)

/*! Convert a number of (8-bit) bytes to a number of bits. */
#define BCMDRD_BYTES2BITS(_x)   ((_x) * 8)

/*! Convert a number of (8-bit) bytes to a number of 32-bit words. */
#define BCMDRD_BYTES2WORDS(_x)  (((_x) + 3) / 4)

/*! Convert a number of 32-bit words to a number of bits. */
#define BCMDRD_WORDS2BITS(_x)   ((_x) * 32)

/*! Convert a number of 32-bit words to a number of (8-bit) bytes. */
#define BCMDRD_WORDS2BYTES(_x)  ((_x) * 4)

/*! Align a size to a specific number of bytes. */
#define BCMDRD_ALIGN(_s, _a)    (((_s) + ((_a) - 1)) & ~((_a) - 1))

/*! Maximum size of physical table entry (in words). */
#define BCMDRD_MAX_PT_WSIZE \
    BCMDRD_BYTES2WORDS(BCMDRD_CONFIG_MAX_PT_ENTRY_SIZE)

/*! Create enumeration values from list of supported devices. */
#define BCMDRD_DEVLIST_ENTRY(_nm,_vn,_dv,_rv,_md,_pi,_bd,_bc,_fn,_cn,_pf,_pd,_r0,_r1) \
    BCMDRD_DEV_T_##_bd,
/*! Enumeration for all base device types. */
typedef enum {
    BCMDRD_DEV_T_NONE = 0,
/*! \cond */
#include <bcmdrd/bcmdrd_devlist.h>
/*! \endcond */
    BCMDRD_DEV_T_COUNT
} bcmdrd_dev_type_t;

/*! Generic ID (enum). */
typedef uint32_t bcmdrd_id_t;

/*! Generic invalid ID value. */
#define BCMDRD_INVALID_ID       ((bcmdrd_id_t)-1)

/*! Invalid register value. */
#define INVALIDr        BCMDRD_INVALID_ID

/*! Invalid memory value. */
#define INVALIDm        BCMDRD_INVALID_ID

/*! Invalid field value. */
#define INVALIDf        BCMDRD_INVALID_ID

/*! Check if an ID is valid, i.e. different from BCMDRD_INVALID_ID. */
#define BCMDRD_ID_VALID(_id) \
    ((_id) != BCMDRD_INVALID_ID)

/*! Device-specific symbol ID (enum). */
typedef bcmdrd_id_t bcmdrd_sid_t;

/*! Device-specific field ID (enum). */
typedef bcmdrd_id_t bcmdrd_fid_t;

/*! Enum for string/value map. */
typedef shr_enum_map_t bcmdrd_enum_map_t;

/*!
 * \brief Port number domain.
 *
 * Port-based registers and memories use different port number domains
 * in their physical address. For example, some registers use the
 * physical port number, some registers use the logical port number
 * and some use a MMU port number.
 *
 * For most devices, each block type use the same port number domain,
 * but there are a few exceptions, so this needs to be a per-reg/mem
 * property.
 */
typedef enum bcmdrd_port_num_domain_e {
    BCMDRD_PND_PHYS = 0,
    BCMDRD_PND_LOGIC = 1,
    BCMDRD_PND_MMU = 2,
    BCMDRD_PND_COUNT
} bcmdrd_port_num_domain_t;

/*!
 * \name Port types.
 * \anchor BCMDRD_PORT_TYPE_xxx
 *
 * Port types are defined as bit masks, such that it is possible to
 * group multiple types into a single one, e.g. CPU and loopback ports
 * could be greoup as internal ports.
 */

/*! \{ */

/*! Port type undefined. */
#define BCMDRD_PORT_TYPE_UNDEF  0

/*! Reserved port (e.g. spare port or other unused port). */
#define BCMDRD_PORT_TYPE_RSVD   (1U << 0)

/*! CPU/HMI port (internal). */
#define BCMDRD_PORT_TYPE_CPU    (1U << 1)

/*! Loopback port (internal). */
#define BCMDRD_PORT_TYPE_LB     (1U << 2)

/*! Front-panel port. */
#define BCMDRD_PORT_TYPE_FPAN   (1U << 3)

/*! Up-link port. */
#define BCMDRD_PORT_TYPE_UPLINK (1U << 4)

/*! Management port. */
#define BCMDRD_PORT_TYPE_MGMT   (1U << 5)

/*! RDB port. */
#define BCMDRD_PORT_TYPE_RDB    (1U << 6)

/*! FAE port. */
#define BCMDRD_PORT_TYPE_FAE    (1U << 7)

/*! AUX port. */
#define BCMDRD_PORT_TYPE_AUX    (1U << 8)
/*! } */

/*!
 * \brief Port category.
 *
 * Please refer to \ref BCMDRD_PORT_TYPE_xxx for a list possible
 * values (categories).
 *
 * Values are bit-based such that a port can belong to multiple
 * categories.
 */
typedef uint32_t bcmdrd_port_type_t;

/*! Words in pipe maps */
#define BCMDRD_PIPEMAP_WORD_MAX (((BCMDRD_CONFIG_MAX_PIPES - 1) >> 5) + 1)

/*!
 * Bitmap of pipes of a particular type.
 */
typedef struct bcmdrd_pipemap_s {
    /*! Word array. */
    uint32_t w[BCMDRD_PIPEMAP_WORD_MAX];
} bcmdrd_pipemap_t;

/* pipe map helper functions */

/*!
 * \brief Check if pipe map is empty.
 *
 * Check that no bits are set in a pipe map of type \ref
 * bcmdrd_pipemap_t.
 *
 * \param [in] pm pipe map.
 *
 * \retval true Pipe map is empty.
 * \retval false Pipe map is not empty.
 */
extern bool
bcmdrd_pipemap_is_null(const bcmdrd_pipemap_t *pm);

/*!
 * \brief Get bitmap word index for a given pipe.
 *
 * A pipe map is an array of data words, and this macro will return
 * the index of the data word associated with a given pipe number.
 *
 * No range check is performed on the pipe number.
 *
 * \param [in] _pm Pipe map.
 * \param [in] _pipe Pipe number to check.
 */
#define BCMDRD_PIPEMAP_WORD(_pm, _pipe)          \
    (&(_pm))->w[(_pipe) >> 5]

/*!
 * \brief Check if a pipe is member of a pipe map.
 *
 * Check if a pipe is member of a pipe map of type \ref
 * bcmdrd_pipemap_t.
 *
 * No range check is performed on the pipe number.
 *
 * \param [in] _pm Pipe map.
 * \param [in] _pipe Pipe number to check.
 */
#define BCMDRD_PIPEMAP_MEMBER(_pm, _pipe) \
     (BCMDRD_PIPEMAP_WORD(_pm, _pipe) & LSHIFT32(1, (_pipe) & 0x1f))

/*!
 * \brief Iterate over a pipe map.
 *
 * Iterate over a pipe map of type \ref bcmdrd_pipemap_t and
 * execute the subsequent statement for all bits set in the pipe
 * bitmap.
 *
 * \param [in] _pm Pipe map.
 * \param [out] _pipe Pipe iterator variable.
 */
#define BCMDRD_PIPEMAP_ITER(_pm, _pipe)                         \
    for (_pipe = 0; _pipe < BCMDRD_CONFIG_MAX_PIPES; _pipe++)   \
        if (BCMDRD_PIPEMAP_WORD(_pm, _pipe) == 0)               \
            _pipe += 31;                                        \
        else if (BCMDRD_PIPEMAP_MEMBER(_pm, _pipe))

/*!
 * \brief Iterate over a pipe map with maximum.
 *
 * Iterate over a pipe map of type \ref bcmdrd_pipemap_t and
 * execute the subsequent statement for all bits set in the pipe
 * bitmap.
 *
 * The iteration will be terminated if the iterator variable reaches
 * the value of \c _pipe_max. This macro is mainly intended to prevent
 * false errors from static analysis tools like Coverity.
 *
 * \param [in] _pm Pipe map.
 * \param [in] _pipe_max Maximum number of pipes.
 * \param [out] _pipe Pipe iterator variable.
 */
#define BCMDRD_PIPEMAP_MAX_ITER(_pm, _pipe_max, _pipe)          \
    for (_pipe = 0;                                             \
         _pipe < _pipe_max && _pipe < BCMDRD_CONFIG_MAX_PIPES;  \
         _pipe++)                                               \
        if (BCMDRD_PIPEMAP_WORD(_pm, _pipe) == 0)               \
            _pipe += 31;                                        \
        else if (BCMDRD_PIPEMAP_MEMBER(_pm, _pipe))

/*!
 * \brief Add a pipe to a pipe map.
 *
 * Add a pipe to a pipe map of type \ref bcmdrd_pipemap_t.
 *
 * No range check is performed on the pipe number.
 *
 * \param [in] _pm Pipe map.
 * \param [in] _pipe Pipe number to add.
 */
#define BCMDRD_PIPEMAP_PIPE_ADD(_pm, _pipe) \
     (BCMDRD_PIPEMAP_WORD(_pm, _pipe) |= LSHIFT32(1, (_pipe) & 0x1f))

/*!
 * \brief Remove a pipe from a pipe map.
 *
 * Remove a pipe from a pipe map of type \ref bcmdrd_pipemap_t.
 *
 * No range check is performed on the pipe number.
 *
 * \param [in] _pm Pipe map.
 * \param [in] _pipe Pipe number to remove.
 */
#define BCMDRD_PIPEMAP_PIPE_REMOVE(_pm, _pipe) \
     (BCMDRD_PIPEMAP_WORD(_pm, _pipe) &= ~(LSHIFT32(1, (_pipe) & 0x1f)))

/*!
 * \brief Clear a pipe map.
 *
 * Clear a pipe map of type \ref bcmdrd_pipemap_t.
 *
 * After clearing the pipe map, it will have no members.
 *
 * \param [in] _pm Pipe map.
 */
#define BCMDRD_PIPEMAP_CLEAR(_pm) \
    sal_memset(&_pm, 0, sizeof(bcmdrd_pipemap_t))

/*!
 * \brief Get a word from a pipe map.
 *
 * Get a 32-bit word from a pipe map of type \ref bcmdrd_pipemap_t.
 *
 * \param [in] _pm Pipe map.
 * \param [in] _w Word number to get (first word is word 0).
 */
#define BCMDRD_PIPEMAP_WORD_GET(_pm, _w) \
    ((&(_pm))->w[_w])

/*!
 * \brief Set a word in a pipe map.
 *
 * Set a 32-bit word in a pipe map of type \ref bcmdrd_pipemap_t.
 *
 * \param [in] _pm Pipe map.
 * \param [in] _w Word number to set (first word is word 0).
 * \param [in] _val Value of word to set.
 */
#define BCMDRD_PIPEMAP_WORD_SET(_pm, _w, _val) \
    ((&(_pm))->w[_w]) = (_val)

/*!
 * \brief Helper macro for pipe map operations.
 *
 * \param [in] _pm0 First pipe map.
 * \param [in] _pm1 Second pipe map.
 * \param [in] _op Pipe map operator.
 */
#define BCMDRD_PIPEMAP_BMOP(_pm0, _pm1, _op) \
    do { \
        int _w; \
        for (_w = 0; _w < BCMDRD_PIPEMAP_WORD_MAX; _w++) { \
            BCMDRD_PIPEMAP_WORD_GET(_pm0, _w) _op BCMDRD_PIPEMAP_WORD_GET(_pm1, _w); \
        } \
    } while (0)

/*! Return true if pipe map _pm is empty. */
#define BCMDRD_PIPEMAP_IS_NULL(_pm) \
    (bcmdrd_pipemap_is_null(&(_pm)))

/*! Return true if pipe map _pm is not empty. */
#define BCMDRD_PIPEMAP_NOT_NULL(_pm) \
    (!(bcmdrd_pipemap_is_null(&(_pm))))

/*! Assign pipe map src to pipe map dst. */
#define BCMDRD_PIPEMAP_ASSIGN(dst, src) \
    sal_memcpy(&(dst), &(src), sizeof(bcmdrd_pipemap_t))

/*!
 * Perform a logical AND operation between all bits of pipe maps _pm0
 * and _pm1.
 */
#define BCMDRD_PIPEMAP_AND(_pm0, _pm1) \
    BCMDRD_PIPEMAP_BMOP(_pm0, _pm1, &=)

/*!
 * Perform a logical OR operation between all bits of pipe maps _pm0
 * and _pm1.
 */
#define BCMDRD_PIPEMAP_OR(_pm0, _pm1) \
    BCMDRD_PIPEMAP_BMOP(_pm0, _pm1, |=)

/*!
 * Perform a logical XOR operation between all bits of pipe maps _pm0
 * and _pm1.
 */
#define BCMDRD_PIPEMAP_XOR(_pm0, _pm1) \
    BCMDRD_PIPEMAP_BMOP(_pm0, _pm1, ^=)

/*!
 * Remove all bits in pipe map _pm1 from pipe map _pm0.
 */
#define BCMDRD_PIPEMAP_REMOVE(_pm0, _pm1) \
    BCMDRD_PIPEMAP_BMOP(_pm0, _pm1, &= ~)

/*!
 * Assign an inversed pipe map _pm1 to pipe map _pm0, i.e. any pipe
 * which is a member of _pm1 will not be a member of _pm0 and vice
 * versa.
 */
#define BCMDRD_PIPEMAP_NEGATE(_pm0, _pm1) \
    BCMDRD_PIPEMAP_BMOP(_pm0, _pm1, = ~)

#endif /* BCMDRD_TYPES_H */
