/*! \file bcmdrd_symbol_types.h
 *
 * <description>
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

#ifndef BCMDRD_SYMBOL_TYPES_H
#define BCMDRD_SYMBOL_TYPES_H

#include <bcmdrd/bcmdrd_types.h>

/*!
 * \name Symbol flags.
 * \anchor BCMDRD_SYMBOL_FLAG_xxx
 */

/*! \{ */

/*! Symbol applicable for SER test. */
#define BCMDRD_SYMBOL_FLAG_SER_TEST                         (1U << 7)

/*! Symbol data will be cleared on read. */
#define BCMDRD_SYMBOL_FLAG_CLEAR_ON_READ                    (1U << 8)

/*! Symbol data can be updated by hardware. */
#define BCMDRD_SYMBOL_FLAG_HW_UPDATED                       (1U << 9)

/*! Symbol should not be accessed with traffic running. */
#define BCMDRD_SYMBOL_FLAG_ACC_IDLE                         (1U << 10)

/*! Symbol validity can be decided dynamically at system initialization. */
#define BCMDRD_SYMBOL_FLAG_DYNAMIC                          (1U << 11)

/*! Symbol represents a special access. */
#define BCMDRD_SYMBOL_FLAG_SPECIAL                          (1U << 12)

/*! Symbol access type represents a unique pipe. */
#define BCMDRD_SYMBOL_FLAG_ACC_UNIQUE_PIPE                  (1U << 13)

/*! Symbol is associated with a specific sub-pipe. */
#define BCMDRD_SYMBOL_FLAG_SUB_PIPE                         (1U << 14)

/*! Symbol is write-only. */
#define BCMDRD_SYMBOL_FLAG_WRITEONLY                        (1U << 15)

/*! Symbol is not suitable for read/write tests. */
#define BCMDRD_SYMBOL_FLAG_NOTEST                           (1U << 16)

/*! Symbol is a non-CMIC iProc register. */
#define BCMDRD_SYMBOL_FLAG_IPROC                            (1U << 17)

/*! Symbol is an overlay of other symbols. */
#define BCMDRD_SYMBOL_FLAG_OVERLAY                          (1U << 18)

/*! Symbol is read-only or any field within the symbol is read-only. */
#define BCMDRD_SYMBOL_FLAG_READONLY                         (1U << 19)

/*! Symbol with FIFO operations. */
#define BCMDRD_SYMBOL_FLAG_FIFO                             (1U << 20)

/*! Symbol is reasonable to cache in S/W. */
#define BCMDRD_SYMBOL_FLAG_CACHEABLE                        (1U << 21)

/*! Symbol is a hashed table. */
#define BCMDRD_SYMBOL_FLAG_HASHED                           (1U << 22)

/*! Symbol is an external CAM. */
#define BCMDRD_SYMBOL_FLAG_EXT_CAM                          (1U << 23)

/*! Symbol is a CAM. */
#define BCMDRD_SYMBOL_FLAG_CAM                              (1U << 24)

/*! Symbol is a register. */
#define BCMDRD_SYMBOL_FLAG_REGISTER                         (1U << 25)

/*! Symbol is a port-based register, i.e. one ionstance per port. */
#define BCMDRD_SYMBOL_FLAG_PORT                             (1U << 26)

/*! Symbol is a counter register. */
#define BCMDRD_SYMBOL_FLAG_COUNTER                          (1U << 27)

/*! Symbol is a memory. */
#define BCMDRD_SYMBOL_FLAG_MEMORY                           (1U << 28)

/*! Symbol uses big endian word ordering. */
#define BCMDRD_SYMBOL_FLAG_BIG_ENDIAN                       (1U << 29)

/*! Symbol is a memory-mapped register. */
#define BCMDRD_SYMBOL_FLAG_MEMMAPPED                        (1U << 30)

/*! Symbol is a port-block register, i.e. one instance per port-block. */
#define BCMDRD_SYMBOL_FLAG_SOFT_PORT                        (1U << 31)

/*! \} */

/*!
 * \name Symbol attributes.
 * \anchor BCMDRD_SYM_ATTR_xxx
 */

/*! \{ */

/*! Symbol is a CAM. */
#define BCMDRD_SYM_ATTR_CAM                                 (1 << 0)

/*! Symbol is a hashed memory. */
#define BCMDRD_SYM_ATTR_HASHED                              (1 << 1)

/*! Symbol is not visible in this configuration. */
#define BCMDRD_SYM_ATTR_HIDDEN                              (1 << 2)

/*! \} */

/*!
 * \brief Symbol (register/memory) information
 *
 * The symbol information is dynamic information about a symbol in the
 * current device configuraton. In many situations, this information
 * will be identical to the static symbol information of the base
 * device.
 */
typedef struct {

    /*! Symbol ID (unique per device). */
    bcmdrd_sid_t sid;

    /*! Symbol name, e.g. "VLANm" or "MISCCONFIGr". */
    const char *name;

    /*! Mask of block types this symbol is valid for. */
    uint32_t blktypes;

    /*! Special attributes of this symbol (\ref BCMDRD_SYMBOL_FLAG_xxx). */
    uint32_t flags;

    /*! Fixed part of symbol address (composition is device-dependent). */
    uint32_t offset;

    /*! Minimum valid index for array-type symbols. */
    uint32_t index_min;

    /*! Maximum valid index for array-type symbols. */
    uint32_t index_max;

    /*! Size of symbol data (or entry if array-type) in 32-bit words. */
    uint32_t entry_wsize;

    /*! Index address step value for array-type symbols. */
    uint32_t step_size;

} bcmdrd_sym_info_t;

/*!
 * \name Symbol field flags.
 * \anchor BCMDRD_SYMBOL_FIELD_FLAG_xxx
 */

/*! \{ */

/*! Symbol field is a counter field. */
#define BCMDRD_SYMBOL_FIELD_FLAG_COUNTER                    (1 << 0)

/*! Symbol field is a key field. */
#define BCMDRD_SYMBOL_FIELD_FLAG_KEY                        (1 << 1)

/*! Symbol field is a mask field. */
#define BCMDRD_SYMBOL_FIELD_FLAG_MASK                       (1 << 2)

/*! \} */

/*!
 * \brief Field information structure.
 *
 * This structure defines a single field within a symbol.
 */
typedef struct bcmdrd_sym_field_info_s {

    /*! Field name, e.g. "VLANf" or "VALIDf". */
    const char *name;

    /*! Field ID (unique per device). */
    int fid;

    /*! Special field ID, which defines how multi-view memories are decoded. */
    int view;

    /*! Special attributes of this field (\ref BCMDRD_SYMBOL_FIELD_FLAG_xxx). */
    uint32_t flags;

    /*! First bit of this field. */
    uint16_t minbit;

    /*! Last bit of this field. */
    uint16_t maxbit;

} bcmdrd_sym_field_info_t;

#endif /* BCMDRD_SYMBOL_TYPES_H */
