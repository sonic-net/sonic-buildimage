/*! \file bcmdrd_symbols.h
 *
 * Chip symbol table definitions.
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

#ifndef BCMDRD_SYMBOLS_H
#define BCMDRD_SYMBOLS_H

/*******************************************************************************
 * BCMDRD symbol formats
 ******************************************************************************/

#include <bcmdrd/bcmdrd_symbol_types.h>

/*******************************************************************************
 * Resolve compile depedencies for optimized code size reduction.
 ******************************************************************************/

#if BCMDRD_CONFIG_INCLUDE_CHIP_SYMBOLS == 0
#undef BCMDRD_CONFIG_INCLUDE_FIELD_INFO
#define BCMDRD_CONFIG_INCLUDE_FIELD_INFO 0
#endif

/*!
 * \name Symbol index information.
 * \anchor BCMDRD_SYMBOL_INDEX_F_xxx
 *
 * In order to maintain a compact symbol table format the entry size
 * and the minimum/maximum table indexes are encoded into a single
 * 32-bit word.
 *
 * The following flags and macros are used to encode and extract the
 * above information to/from the 32-bit index word.
 */

/*! \{ */

/*!
 * \brief Flag indicating exponential index calculation.
 *
 * Used if real maximum cannot be contained within max field.
 *
 * Real maximum is encoded as ((1 << min) * max) - 1).
 *
 * Real minimum is always zero.
 *
 * Example:
 * (max,min)=(17,12)
 *  => real max = ((1 << 12) * 17) - 1) = 0x10fff
 */
#define BCMDRD_SYMBOL_INDEX_F_EXP  0x1

/*!
 * \brief Flag indicating variable index maximum.
 *
 * Used e.g. if register array size is port-dependent.
 *
 * Real maximum and minimum depend on configuration.
 *
 * The min value is used as an encoding key which may be passed to a
 * chip-specific function.
 */
#define BCMDRD_SYMBOL_INDEX_F_VAR  0x2

/*!
 * \brief Flag indicating address step different from 1.
 *
 * Used e.g. if address step for a register array is non-standard.
 *
 * Real minimum is always zero.
 *
 * The min value is interpreted as (address step - 1).
 *
 * Example:
 * (max,min)=(11,3)
 *  => 12 elements with addr(i+1) = addr(i)+4
 */
#define BCMDRD_SYMBOL_INDEX_F_STEP 0x4

/*! Encode entry size into index word. */
#define BCMDRD_SYMBOL_INDEX_SIZE_ENCODE(s) F32_ENCODE((uint32_t)s, 0, 8)

/*! Extract entry size from index word. */
#define BCMDRD_SYMBOL_INDEX_SIZE_GET(w) F32_GET(w, 0, 8)

/*! Encode minimum table index into index word. */
#define BCMDRD_SYMBOL_INDEX_MIN_ENCODE(s) F32_ENCODE((uint32_t)s, 8, 5)

/*! Extract raw minimum table index from index word. */
#define BCMDRD_SYMBOL_INDEX_MIN_GETRAW(w) F32_GET(w, 8, 5)

/*! Encode maximum table index into index word. */
#define BCMDRD_SYMBOL_INDEX_MAX_ENCODE(s) F32_ENCODE((uint32_t)s, 13, 16)

/*! Extract raw maximum table index from index word. */
#define BCMDRD_SYMBOL_INDEX_MAX_GETRAW(w) F32_GET(w, 13, 16)

/*! Encode index flags into index word. */
#define BCMDRD_SYMBOL_INDEX_FLAGS_ENCODE(s) F32_ENCODE((uint32_t)s, 29, 3)

/*! Extract index flags from index word. */
#define BCMDRD_SYMBOL_INDEX_FLAGS_GET(w) F32_GET(w, 29, 3)

/*! Get real minimum table index based on raw values and flags. */
#define BCMDRD_SYMBOL_INDEX_MIN_GET(w) \
    (BCMDRD_SYMBOL_INDEX_FLAGS_GET(w) ? \
       0 : \
       BCMDRD_SYMBOL_INDEX_MIN_GETRAW(w))

/*! Get encoding type from index word. */
#define BCMDRD_SYMBOL_INDEX_ENC_GET(w) BCMDRD_SYMBOL_INDEX_MIN_GETRAW(w)

/*! Get real maximum table index based on raw values and flags. */
#define BCMDRD_SYMBOL_INDEX_MAX_GET(w) \
    ((BCMDRD_SYMBOL_INDEX_FLAGS_GET(w) & BCMDRD_SYMBOL_INDEX_F_EXP) ? \
       (BCMDRD_SYMBOL_INDEX_MAX_GETRAW(w) * \
        (1U << BCMDRD_SYMBOL_INDEX_MIN_GETRAW(w))) - 1 : \
       BCMDRD_SYMBOL_INDEX_MAX_GETRAW(w))

/*! Get per-index address step value. */
#define BCMDRD_SYMBOL_INDEX_STEP_GET(w) \
    ((BCMDRD_SYMBOL_INDEX_FLAGS_GET(w) & BCMDRD_SYMBOL_INDEX_F_STEP) ? \
       (BCMDRD_SYMBOL_INDEX_MIN_GETRAW(w) + 1) : \
       1)

/*! \} */

/*!
 * \name Symbol field encode information.
 *
 * Symbol field information is encoded in variable size. The supported formats
 * are depicted in the following tables:
 *
 * - Single Word Field Format
 * | Bits  | Name      | Description                                 |
 * | :---: | --------- | ------------------------------------------- |
 * | 31:31 | LastField | Indicates this is the last field descriptor word in a field array. |
 * | 30:30 | Ext       | Indicates that this is the first word in a Double Word Field, not a Single Word Field.<br>This word and the next form a Double Word Field. |
 * | 29:29 | Ext2      | Indicates that this field is visible only in certain memory views.<br>This word and the next two form a Triple Word Field. |
 * | 28:15 | FieldId   | This is the unique field id for this field. |
 * | 14:7  | Minbit    | This is the field's minbit.                 |
 * | 6:0   | Width     | This is the field's width (in bits).        |
 * <br>
 *
 * - Double Word Field Format
 * | Word | Bits  | Name      | Description                                 |
 * | :--: | :---: | --------- | ------------------------------------------- |
 * | 0    | 31:31 | LastField | Indicates this is the last field descriptor word in a field array. |
 * | 0    | 30:30 | Ext       | Indicates this is the start of a Double Word field. |
 * | 0    | 29:29 | Ext2      | Must be zero for Double Word fields.        |
 * | 0    | 28:28 | Cnt       | Indicates that field is a counter.          |
 * | 0    | 27:27 | Key       | Indicates that field is a key.              |
 * | 0    | 26:26 | Mask      | Indicates that field is a mask.             |
 * | 0    | 25:24 | Rsvd      | Reserved for future field attributes.       |
 * | 0    | 23:0  | FieldId   | This is the unique field id for this field. |
 * | 1    | 31:16 | Minbit    | This is the field's minbit.                 |
 * | 1    | 15:0  | Width     | This is the field's width (in bits).        |
 * <br>
 *
 * - Triple Word Field Format
 * | Word | Bits  | Name      | Description                                 |
 * | :--: | :---: | --------- | ------------------------------------------- |
 * | 0    | 31:31 | LastField | Indicates this is the last field descriptor word in a field array. |
 * | 0    | 30:30 | Ext       | Must be zero for Triple Word fields.        |
 * | 0    | 29:29 | Ext2      | Indicates that this word and the next two form a Triple Word Field. |
 * | 0    | 28:28 | Cnt       | Indicates that field is a counter.          |
 * | 0    | 27:27 | Key       | Indicates that field is a key.              |
 * | 0    | 26:26 | Mask      | Indicates that field is a mask.             |
 * | 0    | 25:24 | Rsvd      | Reserved for future field attributes.       |
 * | 0    | 23:0  | FieldId   | This is the unique field id for this field. |
 * | 1    | 31:16 | Minbit    | This is the field's minbit.                 |
 * | 1    | 15:0  | Width     | This is the field's width (in bits).        |
 * | 2    | 23:0  | ViewId    | This is the unique field view id for this field. |
 * <br>
 */

/*! \{ */

/*! Indicates last field descriptor word in a field array. */
#define BCMDRD_SYM_FIELD_FLAG_LAST      (1U << 31)

/*! Indicates double-word field descriptor. */
#define BCMDRD_SYM_FIELD_FLAG_EXT       (1U << 30)

/*! Indicates triple-word field descriptor. */
#define BCMDRD_SYM_FIELD_FLAG_EXT2      (1U << 29)

/*! Indicates counter attribute in a field */
#define BCMDRD_SYM_FIELD_ATTR_COUNTER   (1U << 28)

/*! Indicates key attribute in a field */
#define BCMDRD_SYM_FIELD_ATTR_KEY       (1U << 27)

/*! Indicates mask attribute in a field */
#define BCMDRD_SYM_FIELD_ATTR_MASK      (1U << 26)

/*! Test field descriptor word for \ref BCMDRD_SYM_FIELD_FLAG_LAST. */
#define BCMDRD_SYM_FIELD_LAST(w) ((w) & BCMDRD_SYM_FIELD_FLAG_LAST)

/*! Test field descriptor word for \ref BCMDRD_SYM_FIELD_FLAG_EXT. */
#define BCMDRD_SYM_FIELD_EXT(w) ((w) & BCMDRD_SYM_FIELD_FLAG_EXT)

/*! Test field descriptor word for \ref BCMDRD_SYM_FIELD_FLAG_EXT2. */
#define BCMDRD_SYM_FIELD_EXT2(w) ((w) & BCMDRD_SYM_FIELD_FLAG_EXT2)


/*! Encode field ID into a single-word field descriptor. */
#define BCMDRD_SYM_FIELD_ID_ENCODE(id) F32_ENCODE(id, 15, 14)

/*! Encode field start-bit into a single-word field descriptor. */
#define BCMDRD_SYM_FIELD_MIN_ENCODE(b) F32_ENCODE(b, 7, 8)

/*! Encode field size into a single-word field descriptor. */
#define BCMDRD_SYM_FIELD_WIDTH_ENCODE(b) F32_ENCODE(b, 0, 7)

/*! Extract field ID from a single-word field descriptor. */
#define BCMDRD_SYM_FIELD_ID_GET(w) F32_GET(w, 15, 14)

/*! Extract field start-bit from a single-word field descriptor. */
#define BCMDRD_SYM_FIELD_MIN_GET(w) F32_GET(w, 7, 8)

/*! Extract field size from a single-word field descriptor. */
#define BCMDRD_SYM_FIELD_WIDTH_GET(w) F32_GET(w, 0, 7)

/*! Encode field information into a single-word field descriptor. */
#define BCMDRD_SYM_FIELD_ENCODE(id, min, width)                         \
    (BCMDRD_SYM_FIELD_ID_ENCODE(id) |                                   \
     BCMDRD_SYM_FIELD_MIN_ENCODE(min) |                                 \
     BCMDRD_SYM_FIELD_WIDTH_ENCODE(width))

/*! Encode field ID into a double-word field descriptor. */
#define BCMDRD_SYM_FIELD_EXT_ID_ENCODE(id) F32_ENCODE(id, 0, 24)

/*! Encode field start-bit into a double-word field descriptor. */
#define BCMDRD_SYM_FIELD_EXT_MIN_ENCODE(b) F32_ENCODE(b, 16, 16)

/*! Encode field size into a double-word field descriptor. */
#define BCMDRD_SYM_FIELD_EXT_WIDTH_ENCODE(b) F32_ENCODE(b, 0, 16)

/*! Extract field ID from a double-word field descriptor. */
#define BCMDRD_SYM_FIELD_EXT_ID_GET(w) F32_GET(w, 0, 24)

/*! Extract field start-bit from a double-word field descriptor. */
#define BCMDRD_SYM_FIELD_EXT_MIN_GET(w) F32_GET((&(w))[1], 16, 16)

/*! Extract field size from a double-word field descriptor. */
#define BCMDRD_SYM_FIELD_EXT_WIDTH_GET(w) F32_GET((&(w))[1], 0, 16)

/*! Encode field information into a double-word field descriptor. */
#define BCMDRD_SYM_FIELD_EXT_ENCODE(id, min, width)                     \
    (BCMDRD_SYM_FIELD_FLAG_EXT | BCMDRD_SYM_FIELD_EXT_ID_ENCODE(id)),   \
        (BCMDRD_SYM_FIELD_EXT_MIN_ENCODE(min) |                         \
         BCMDRD_SYM_FIELD_EXT_WIDTH_ENCODE(width))

/*!
 * Test field attribute for \ref BCMDRD_SYM_FIELD_ATTR_COUNTER
 * in double-word field descriptor.
 */
#define BCMDRD_SYM_FIELD_EXT_ATTR_IS_COUNTER(w)                         \
    ((w) & BCMDRD_SYM_FIELD_ATTR_COUNTER)

/*!
 * Test field attribute for \ref BCMDRD_SYM_FIELD_ATTR_KEY
 * in double-word field descriptor.
 */
#define BCMDRD_SYM_FIELD_EXT_ATTR_IS_KEY(w)                             \
    ((w) & BCMDRD_SYM_FIELD_ATTR_KEY)

/*!
 * Test field attribute for \ref BCMDRD_SYM_FIELD_ATTR_MASK
 * in double-word field descriptor.
 */
#define BCMDRD_SYM_FIELD_EXT_ATTR_IS_MASK(w)                            \
    ((w) & BCMDRD_SYM_FIELD_ATTR_MASK)


/*! Encode field ID into a triple-word field descriptor. */
#define BCMDRD_SYM_FIELD_EXT2_ID_ENCODE(id) F32_ENCODE(id, 0, 24)

/*! Encode field start-bit into a triple-word field descriptor. */
#define BCMDRD_SYM_FIELD_EXT2_MIN_ENCODE(b) F32_ENCODE(b, 16, 16)

/*! Encode field size into a triple-word field descriptor. */
#define BCMDRD_SYM_FIELD_EXT2_WIDTH_ENCODE(b) F32_ENCODE(b, 0, 16)

/*! Encode field view ID into a triple-word field descriptor. */
#define BCMDRD_SYM_FIELD_EXT2_VIEW_ENCODE(b) F32_ENCODE(b, 0, 24)

/*! Extract field ID from a triple-word field descriptor. */
#define BCMDRD_SYM_FIELD_EXT2_ID_GET(w) F32_GET(w, 0, 24)

/*! Extract field start-bit from a triple-word field descriptor. */
#define BCMDRD_SYM_FIELD_EXT2_MIN_GET(w) F32_GET((&(w))[1], 16, 16)

/*! Extract field width from a triple-word field descriptor. */
#define BCMDRD_SYM_FIELD_EXT2_WIDTH_GET(w) F32_GET((&(w))[1], 0, 16)

/*! Extract field view ID from a triple-word field descriptor. */
#define BCMDRD_SYM_FIELD_EXT2_VIEW_GET(w) F32_GET((&(w))[2], 0, 24)

/*! Encode field information into a triple-word field descriptor. */
#define BCMDRD_SYM_FIELD_EXT2_ENCODE(id, min, width, view)              \
    (BCMDRD_SYM_FIELD_FLAG_EXT2 | BCMDRD_SYM_FIELD_EXT2_ID_ENCODE(id)), \
        (BCMDRD_SYM_FIELD_EXT2_MIN_ENCODE(min) |                        \
         BCMDRD_SYM_FIELD_EXT2_WIDTH_ENCODE(width)),                    \
        BCMDRD_SYM_FIELD_EXT2_VIEW_ENCODE(view)

/*!
 * Test field attribute for \ref BCMDRD_SYM_FIELD_ATTR_COUNTER
 * in triple-word field descriptor.
 */
#define BCMDRD_SYM_FIELD_EXT2_ATTR_IS_COUNTER(w)                        \
    ((w) & BCMDRD_SYM_FIELD_ATTR_COUNTER)

/*!
 * Test field attribute for \ref BCMDRD_SYM_FIELD_ATTR_KEY
 * in triple-word field descriptor.
 */
#define BCMDRD_SYM_FIELD_EXT2_ATTR_IS_KEY(w)                            \
    ((w) & BCMDRD_SYM_FIELD_ATTR_KEY)

/*!
 * Test field attribute for \ref BCMDRD_SYM_FIELD_ATTR_MASK
 * in triple-word field descriptor.
 */
#define BCMDRD_SYM_FIELD_EXT2_ATTR_IS_MASK(w)                           \
    ((w) & BCMDRD_SYM_FIELD_ATTR_MASK)

/*! \} */


/*!
 * \brief Iterate over all fields for a symbol.
 *
 * Given a pointer to a field descriptor word array, this macro will
 * fill out the \ref bcmdrd_sym_field_info_t structure for each field
 * in the field descriptor word array.
 *
 * If a list of corresponding fieln names is given, the field
 * information will include the symbolic field name.
 *
 * The macro must be terminated with \ref BCMDRD_SYM_FIELDS_ITER_END.
 */
#define BCMDRD_SYM_FIELDS_ITER_BEGIN(start, finfo, fnames) {   \
    uint32_t *_fp = start;                                     \
    for (;;) {                                                 \
        if (!_fp) {                                            \
            break;                                             \
        } else {                                               \
            _fp = bcmdrd_sym_field_info_decode(_fp, (fnames), &(finfo));

/*! Refer to \ref BCMDRD_SYM_FIELDS_ITER_BEGIN. */
#define BCMDRD_SYM_FIELDS_ITER_END() }}}

/*!
 * Symbol information structure (single symbol).
 */
typedef struct bcmdrd_symbol_s {

    /*! Base offset (fixed address information). */
    uint32_t addr;

    /*! Encoded information about valid indexes for arrays. */
    uint32_t index;

    /*! Symbol flags (\ref BCMDRD_SYMBOL_FLAG_xxx). */
    uint32_t flags;

    /*!
     * Device-specific information.
     * | Bits  | Name              | Description                              |
     * | :---: | ----------------- | ---------------------------------------- |
     * | 31:24 | HMI-specific data | This is 8-bit symbol HMI-specific data.  |
     * | 23:21 | Sub-pipe instance | Sub-pipe association (if any).           |
     * | 20:14 | Access type       | This is the symbol access type.          |
     * | 13:7  | Block type 1      | The 2nd symbol block type (> 0 if any).  |
     * |  6:0  | Block type 0      | The 1st symbol block type (> 0 always).  |
     */
    uint32_t info;

    /*! Profile information for default value, SER information, etc. */
    uint32_t pfinfo;

#if BCMDRD_CONFIG_INCLUDE_FIELD_INFO == 1
    /*! Pointer to field information data. */
    uint32_t *fields;
#endif

    /*! Symbol name. */
    const char *name;

#if BCMDRD_CONFIG_INCLUDE_ALIAS_NAMES == 1
    /*!
     * Alternative symbol name (typically used if hardware and
     * software names differ).
     */
    const char *alias;

    /*!
     * User-fiendly symbol name (typically a shorter a more meaningful
     * name).
     */
    const char *ufname;
#endif

} bcmdrd_symbol_t;

/*! Maximum block types encoded in the symbol device-specific information. */
#define BCMDRD_SYM_INFO_MAX_BLKTYPES 2

/*! Bit number of the encoded block type. */
#define BCMDRD_SYM_INFO_BLKTYPE_BITS 7

/*! Bit mask of the encoded block type. */
#define BCMDRD_SYM_INFO_BLKTYPE_MASK ((1U << BCMDRD_SYM_INFO_BLKTYPE_BITS) - 1)

/*! Bit mask of all the encoded block types. */
#define BCMDRD_SYM_INFO_BLKTYPES_MASK \
    ((1U << (BCMDRD_SYM_INFO_MAX_BLKTYPES * BCMDRD_SYM_INFO_BLKTYPE_BITS)) - 1)

/*! Extract block types from symbol device-specific information. */
#define BCMDRD_SYM_INFO_BLKTYPES(_w) ((_w) & BCMDRD_SYM_INFO_BLKTYPES_MASK)

/*! Bit position of the encoded access type. */
#define BCMDRD_SYM_INFO_ACCTYPE_SHIFT \
    (BCMDRD_SYM_INFO_MAX_BLKTYPES * BCMDRD_SYM_INFO_BLKTYPE_BITS)

/*! Bit number of the encoded access type. */
#define BCMDRD_SYM_INFO_ACCTYPE_BITS 7

/*! Bit mask of the encoded access type. */
#define BCMDRD_SYM_INFO_ACCTYPE_MASK ((1U << BCMDRD_SYM_INFO_ACCTYPE_BITS) - 1)

/*! Extract access type from symbol device-specific information. */
#define BCMDRD_SYM_INFO_ACCTYPE(_w) \
    (((_w) >> BCMDRD_SYM_INFO_ACCTYPE_SHIFT) & BCMDRD_SYM_INFO_ACCTYPE_MASK)

/*! Bit postion of the encoded sub-pipe instance. */
#define BCMDRD_SYM_INFO_SUBPIPE_INST_SHIFT \
    (BCMDRD_SYM_INFO_ACCTYPE_SHIFT + BCMDRD_SYM_INFO_ACCTYPE_BITS)

/*! Bit number of the encoded sub-pipe instance. */
#define BCMDRD_SYM_INFO_SUBPIPE_INST_BITS 3

/*! Bit mask of the encoded sub-pipe instance. */
#define BCMDRD_SYM_INFO_SUBPIPE_INST_MASK \
    ((1U << BCMDRD_SYM_INFO_SUBPIPE_INST_BITS) - 1)

/*! Extract sub-pipe instance from symbol device-specific information. */
#define BCMDRD_SYM_INFO_SUBPIPE_INST(_w) \
    (((_w) >> BCMDRD_SYM_INFO_SUBPIPE_INST_SHIFT) & \
     BCMDRD_SYM_INFO_SUBPIPE_INST_MASK)

/*! Extract block type by index from symbol device-specific information. */
#define BCMDRD_SYM_INFO_BLKTYPE(_w, _i) \
    ((BCMDRD_SYM_INFO_BLKTYPES(_w) >> \
     (BCMDRD_SYM_INFO_BLKTYPE_BITS * (_i))) & BCMDRD_SYM_INFO_BLKTYPE_MASK)

/*!
 * \name Symbol profile information.
 */

/*! \{ */

/*! Set profile offset for memory symbol. */
#define BCMDRD_SYM_MEMPF_OFFSET_SET(_p) F32_ENCODE((_p), 0, 12)

/*! Get profile offset for memory. */
#define BCMDRD_SYM_MEMPF_OFFSET_GET(_pi) F32_GET((_pi), 0, 12)

/*! Set profile offset for register. */
#define BCMDRD_SYM_REGPF_OFFSET_SET(_p) F32_ENCODE((_p), 0, 12)

/*! Get profile offset for register. */
#define BCMDRD_SYM_REGPF_OFFSET_GET(_pi) F32_GET((_pi), 0, 12)

/*! Set profile offset for MOR of symbol. */
#define BCMDRD_SYM_MORPF_OFFSET_SET(_p) F32_ENCODE((_p), 12, 8)

/*! Get profile offset for MOR of symbol. */
#define BCMDRD_SYM_MORPF_OFFSET_GET(_pi) F32_GET((_pi), 12, 8)

/*! Set profile offset for error correction information of symbol. */
#define BCMDRD_SYM_ECCPF_OFFSET_SET(_p) F32_ENCODE((_p), 20, 12)

/*! Get profile offset for error correction information of symbol. */
#define BCMDRD_SYM_ECCPF_OFFSET_GET(_pi) F32_GET((_pi), 20, 12)

/*! \} */


/*!
 * \brief Memory comparison function.
 *
 * \param [in] ent_a Element a of memory to be compared.
 * \param [in] ent_b Element b of memory to be compared.
 *
 * \return 0 if two memory elements equal, otherwise unequal.
 */
typedef int (*bcmdrd_sym_mem_cmp_f)(void *ent_a, void *ent_b);

/*!
 * Memory clear-on-read control structure.
 */
typedef struct bcmdrd_sym_mem_corctrl_s {

    /*! Memory clear-on-read control register. */
    bcmdrd_sid_t sid;

    /*! Memory clear-on-read control field. */
    bcmdrd_fid_t fid;

} bcmdrd_sym_mem_corctrl_t;

/*!
 * Memory profile structure.
 */
typedef struct bcmdrd_sym_mem_profile_s {

    /*! Memory comparison function. */
    bcmdrd_sym_mem_cmp_f cmp_fn;

    /*! Null entry data array. */
    const void *null_ent;

    /*! Memory clear-on-read control. */
    bcmdrd_sym_mem_corctrl_t *cor_ctrl;

} bcmdrd_sym_mem_profile_t;

/*!
 * MOR profile structure.
 */
typedef struct bcmdrd_sym_mor_profile_s {

    /*! MOR read. */
    uint32_t mrd;

    /*! MOR write. */
    uint32_t mwr;

} bcmdrd_sym_mor_profile_t;

/*!
 * Symbols table structure (all symbols).
 */
typedef struct bcmdrd_symbols_s {

    /*! List of all symbols used by this device. */
    const bcmdrd_symbol_t *symbols;

    /*! Number of entries in symbols array. */
    const uint32_t size;

    /*! Number of entries in each symbols array section. */
    const uint32_t sect_size[2];

    /*! List of all field names used by this device. */
    const char **field_names;

    /*! List of all reset values/masks used by this device. */
    const uint32_t *reg_profiles;

    /*! List of all memory profiles used by this device. */
    const bcmdrd_sym_mem_profile_t *mem_profiles;

    /*! List of all MOR profiles used by this device. */
    const bcmdrd_sym_mor_profile_t *mor_profiles;

    /*! SER data used by this device. */
    const void *ser_data;

#if BCMDRD_CONFIG_INCLUDE_ALIAS_NAMES == 1
    /*! List of all symbols alias names used in this device. */
    const bcmdrd_enum_map_t *alias_names;

    /*! Number of entries in symbols alias names array */
    const uint32_t alias_names_size;
#endif

    /*! Symbols are sorted and searchable by bsearch. */
    const bool sorted;

} bcmdrd_symbols_t;


/*!
 * \brief Search a symbol array.
 *
 * Search symbol array by comparing input name to each symbol name.
 *
 * This function is primarily intended for internal use, and should
 * normally not be called directly.
 *
 * \param [in] name Symbol name to search for.
 * \param [in] table Array of symbol structures.
 * \param [in] size Size of symbol array.
 * \param [out] sid Symbol ID if match is found.
 *
 * \retval Pointer to symbol structure, or NULL if not found.
 */
extern const bcmdrd_symbol_t *
bcmdrd_symbol_find(const char *name,
                   const bcmdrd_symbol_t *table, uint32_t size,
                   bcmdrd_sid_t *sid);

/*!
 * \brief Binary search a symbol table.
 *
 * Binary search a symbol table from symbol names or alternative names.
 * The symbol table should be sorted properly for the binary search support.
 *
 * This function is primarily intended for internal use, and should
 * normally not be called directly.
 *
 * \param [in] name Symbol name to look for.
 * \param [in] symbols Symbol table structure.
 * \param [out] sid Symbol ID if match is found.
 *
 * \retval Pointer to symbol structure, or NULL if not found.
 */
const bcmdrd_symbol_t *
bcmdrd_symbols_fast_find(const char *name, const bcmdrd_symbols_t *symbols,
                         bcmdrd_sid_t *sid);

/*!
 * \brief Search a symbol table.
 *
 * Primary symbol search function. Takes a pointer to the symbols
 * structure, finds the name in either one, and returns a full symbol
 * structure.
 *
 * \param [in] name Symbol name to look for.
 * \param [in] symbols Symbol table structure.
 * \param [out] sid Symbol ID if match is found.
 *
 * \retval Pointer to symbol structure, or NULL if not found.
 */
extern const bcmdrd_symbol_t *
bcmdrd_symbols_find(const char *name, const bcmdrd_symbols_t *symbols,
                    bcmdrd_sid_t *sid);

/*!
 * \brief Get a specific symbol by index.
 *
 * \param [in] symbols - Symbols information structure for device.
 * \param [in] sindex - Symbol index (usually same as symbol ID).
 * \param [out] rsym - Symbol information structure to fill.
 *
 * \retval 0 No errors, or -1 if not found
 */
extern int
bcmdrd_symbols_get(const bcmdrd_symbols_t *symbols, uint32_t sindex,
                   bcmdrd_symbol_t *rsym);

/*!
 * \brief Get index of a specific symbol.
 *
 * \param [in] symbols - Symbols information structure for device.
 * \param [in] symbol - Specific symbol structure.
 *
 * \retval Symbol index, or -1 if not found
 */
extern int
bcmdrd_symbols_index(const bcmdrd_symbols_t *symbols,
                     const bcmdrd_symbol_t *symbol);

/*!
 * \brief Get symbol information.
 *
 * Retrieve dynamic symbol information for register/memory.
 *
 * \param [in] symbols - Symbols information structure for device.
 * \param [in] sid Device-specific symbol ID
 * \param [out] sinfo Symbol information structure to fill if not NULL
 *
 * \retval Raw symbol structure, or NULL if not found.
 */
extern const bcmdrd_symbol_t *
bcmdrd_sym_info_get(const bcmdrd_symbols_t *symbols, bcmdrd_sid_t sid,
                    bcmdrd_sym_info_t *sinfo);

/*!
 * \name Symbol matching modes.
 * \anchor BCMDRD_SYMBOLS_ITER_MODE_xxx
 */

/*! \{ */

/*! Symbol table iterator must match exact symbol name. */
#define BCMDRD_SYMBOLS_ITER_MODE_EXACT     0

/*! Symbol table iterator must match beginning of symbol name. */
#define BCMDRD_SYMBOLS_ITER_MODE_START     1

/*! Symbol table iterator must match any sub-string in symbol name. */
#define BCMDRD_SYMBOLS_ITER_MODE_STRSTR    2

/*! \} */

/*!
 * \brief Symbols iteration call-back function.
 *
 * \param [in] symbol Symbol structure (single symbol).
 * \param [in] sid Symbol ID.
 * \param [in] vptr Caller-provided context.
 *
 * \retval 0 on success, -1 on failure.
 */
typedef int (*bcmdrd_symbols_iter_cb_f)(const bcmdrd_symbol_t *symbol,
                                        bcmdrd_sid_t sid,
                                        void *vptr);

/*!
 * \brief Symbol table iterator control structure.
 *
 * This structure contains symbol matching information and call-back
 * function information for the symbol table iterator function.
 */
typedef struct bcmdrd_symbols_iter_s {

    /*! String to match against each symbol. */
    const char *name;

    /*! Defines valid matches (\ref BCMDRD_SYMBOLS_ITER_MODE_xxx) */
    int matching_mode;

    /*! Symbol flags that must be present. */
    uint32_t pflags;

    /*! Symbol flags that must be absent. */
    uint32_t aflags;

    /*!
     * Block types that must be present.
     * List of block types terminated by -1 if not NULL.
     */
    int *pblktypes;

    /*!
     * Block types that must be absent.
     * List of block types terminated by -1 if not NULL.
     */
    int *ablktypes;

    /*!
     * Access types that must be present.
     * List of access types terminated by -1 if not NULL.
     */
    int *pacctypes;

    /*!
     * Access types that must be absent.
     * List of access types terminated by -1 if not NULL.
     */
    int *aacctypes;

    /*! Symbols information structure for device. */
    const bcmdrd_symbols_t *symbols;

    /*! Function to be called for each matching symbol. */
    bcmdrd_symbols_iter_cb_f function;

    /*! Context for call-back function. */
    void *vptr;

} bcmdrd_symbols_iter_t;

/*!
 * \brief Field filtering call-back function.
 *
 * This function will indicate whether a particular field should be
 * filtered out for the current memory view.
 *
 * The field-view encoding string has the following syntax:
 *
 *   {[\<keysrc\>]:\<keyfield\>:\<keyval\>[|\<keyval\> ... ]}
 *
 * Ideally the keysrc is the same data entry which is
 * being decoded, and in this case it can left out, e.g.:
 *
 *   {:KEY_TYPEf:1}
 *
 * This example encoding means that if KEY_TYPEf=1, then
 * this field is valid for this view.
 *
 * Note that a field can be for multiple views, e.g.:
 *
 *   {:KEY_TYPEf:1|3}
 *
 * This example encoding means that this field is valid
 * if KEY_TYPEf=1 or KEY_TYPEf=3.
 *
 * The special \<keyval\>=-1 means that this field is valid
 * even if there is no context (cookie=NULL).
 *
 * \param [in] symbol Symbol structure (single symbol).
 * \param [in] fnames List of all field names for this unit.
 * \param [in] encoding Field-view encoding string.
 * \param [in] cookie Caller-provided context.
 *
 * \retval true if field should be filtered out, otherwise false.
 */
typedef int (*bcmdrd_symbol_filter_cb_f)(const bcmdrd_symbol_t *symbol,
                                         const char **fnames,
                                         const char *encoding,
                                         void *cookie);

/*!
 * \brief Customized symbol filtering call-back function.
 *
 * This function will indicate whether a particular name with
 * with extra flags information should be filtered out.
 *
 * \param [in] symbol Symbol structure (single symbol).
 * \param [in] name Name of the identifier.
 * \param [in] flags Flags carried with the identifier.
 * \param [in] data Table entry data to process.
 * \param [in] cookie Caller-provided context.
 *
 * \retval true if field should be filtered out, otherwise false.
 */
typedef int (*bcmdrd_symbol_custom_filter_cb_f)(const bcmdrd_symbol_t *symbol,
                                                const char *name,
                                                uint32_t flags,
                                                uint32_t *data,
                                                void *cookie);

/*!
 * \brief Iterate over all symbols in a symbol table.
 *
 * Search symbol table and call a user-defined function for each
 * matched symbol. Useful if multiple partial matches may occur.
 *
 * Iteration will stop if the call-back function returns a non-zero
 * value.
 *
 * \param [in] iter Iterator control structure (see \ref
 *                  bcmdrd_symbols_iter_t).
 *
 * \retval 0 No errors, otherwise call-back function return value.
 */
extern int
bcmdrd_symbols_iter(bcmdrd_symbols_iter_t *iter);

/*!
 * \brief Default field-view filtering function.
 *
 * See \ref bcmdrd_symbol_filter_cb_f for a detailed description.
 *
 * \param [in] symbol Symbol structure (single symbol).
 * \param [in] fnames List of all field names for this unit.
 * \param [in] encoding Field-view encoding string.
 * \param [in] cookie Caller-provided context.
 *
 * \retval true if field should be filtered out, otherwise false.
 */
extern int
bcmdrd_symbol_field_filter(const bcmdrd_symbol_t *symbol,
                           const char **fnames,
                           const char *encoding,
                           void *cookie);

/*!
 * \brief Get list of fields IDs for given symbol ID.
 *
 * The function will always return the total number of valid field IDs
 * in num_fid, irrespective of the value of list_max, i.e. if num_fid
 * is greater than list_max, then the returned fid_list was truncated.
 *
 * For example, if list_max is zero, then the number of valid field
 * IDs is returned in num_fid, but the fid_list is not updated (can be
 * specified as NULL). The returned num_fid can then be used to
 * allocate a sufficiently large fid_list array.
 *
 * \param [in] symbols - Symbols information structure for device.
 * \param [in] sid Device-specific symbol ID
 * \param [in] list_max Maximum number of entries in field ID list
 * \param [out] fid_list Field ID list
 * \param [out] num_fid Total number of field IDs for this symbold ID
 *
 * \retval 0 No errors
 */
int
bcmdrd_sym_fid_list_get(const bcmdrd_symbols_t *symbols,
                        bcmdrd_sid_t sid, size_t list_max,
                        bcmdrd_fid_t *fid_list, size_t *num_fid);

/*!
 * \brief Get field information for register/memory field.
 *
 * Obtain field information (name, start bit, end bit, etc.) for
 * specified symbol ID and field ID.
 *
 * \param [in] symbols - Symbols information structure for device.
 * \param [in] sid Device-specific symbol ID
 * \param [in] fid Device-specific field ID
 * \param [out] finfo Field information structure to fill
 *
 * \retval Pointer to symbol entry, or NULL if not found
 */
extern const bcmdrd_symbol_t *
bcmdrd_sym_field_info_get(const bcmdrd_symbols_t *symbols,
                          bcmdrd_sid_t sid, bcmdrd_fid_t fid,
                          bcmdrd_sym_field_info_t *finfo);

/*!
 * \brief Get field information for register/memory field.
 *
 * Obtain field information (name, start bit, end bit, etc.) for
 * specified symbol ID and field name.
 *
 * \param [in] symbols - Symbols information structure for device.
 * \param [in] sid Device-specific symbol ID
 * \param [in] fname Field name (ASCII string)
 * \param [out] finfo Field information structure to fill
 *
 * \return Pointer to symbol entry, or NULL if not found.
 */
extern const bcmdrd_symbol_t *
bcmdrd_sym_field_info_get_by_name(const bcmdrd_symbols_t *symbols,
                                  bcmdrd_sid_t sid, const char *fname,
                                  bcmdrd_sym_field_info_t *finfo);

/*!
 * \brief Decode raw field information.
 *
 * This function decodes raw field information as encoded in the
 * symbol table.
 *
 * FOR INTERNAL USE ONLY.
 *
 * \param [in] fp Pointer to raw field encoding
 * \param [in] fnames Device-specfic list of all field names
 * \param [out] finfo Field information structure
 *
 * \return Pointer to the next field encoding, or NULL if end of list.
 */
extern uint32_t *
bcmdrd_sym_field_info_decode(uint32_t *fp, const char **fnames,
                             bcmdrd_sym_field_info_t *finfo);

/*!
 * \brief Return the number of fields in a register/memory entry.
 *
 * FOR INTERNAL USE ONLY.
 *
 * \param [in] fp Pointer to raw field encoding
 *
 * \return Number of fields in this register/memory entry.
 */
extern uint32_t
bcmdrd_sym_field_info_count(uint32_t *fp);

/*!
 * \brief Common memory comparison function.
 *
 * \param [in] ent_a Element a of memory to be compared.
 * \param [in] ent_b Element b of memory to be compared.
 *
 * \return 0 if two memory elements equal, otherwise unequal.
 */
extern int
bcmdrd_sym_mem_cmp_undef(void *ent_a, void *ent_b);

/*!
 * \brief Memory comparison function with inverted mask.
 *
 * This function will compare only the specified mask bits in
 * memory \c ent_a and \c ent_b. The memory mask will be specified
 * in an inverted bit mask by \c imask. For example, if bits 0~3 are
 * intended to be compared in a 32-bit entry, the contents of \c imask
 * would be 0xfffffff0.
 *
 * \param [in] ent_a Memory entry a to be compared.
 * \param [in] ent_b Memory entry b to be compared.
 * \param [in] imask Invert mask of Memory entry to be masked with memory entry.
 * \param [in] size Compared memory entry size in byte.
 *
 * \return 0 if two values equal, -1 if ent_a < ent_b, and 1 if ent_a > ent_b.
 */
extern int
bcmdrd_sym_mem_cmp_imask(void *ent_a, void *ent_b, void *imask, int size);

/*!
 * \brief Memory comparison function with mask.
 *
 * This function will compare only the specified mask bits in
 * memory \c ent_a and \c ent_b. The memory mask will be specified in an
 * bit mask by \c mask. For example, if bits 0~3 are intended to be compared
 * in a 32-bit entry, the contents of \c mask would be 0x0000000f.
 *
 * \param [in] ent_a Memory entry a to be compared.
 * \param [in] ent_b Memory entry b to be compared.
 * \param [in] mask Mask of Memory entry to be masked with memory entry.
 * \param [in] size Compared memory entry size in byte.
 *
 * \return 0 if two values equal, -1 if ent_a < ent_b, and 1 if ent_a > ent_b.
 */
extern int
bcmdrd_sym_mem_cmp_mask(void *ent_a, void *ent_b, void *mask, int size);

/*!
  * Memory element value comparison macro.
  * The macro will not return only if the comparing values a and b
  * are equal.
  */
#define BCMDRD_SYM_MEM_VAL_CMP_RETURN(_a, _b) { \
    if ((_a) < (_b)) {                          \
        return -1;                              \
    }                                           \
    if ((_a) > (_b)) {                          \
        return  1;                              \
    }                                           \
}

/*!
 * \brief Get comparison function of a specified memory symbol.
 *
 * The comparison function is stored in \c profile of \ref bcmdrd_symbol_t.
 * And \ref bcmdrd_sym_mem_profile_t is used for profile structure of
 * memory symbols. The function is mainly to retrieve the memory
 * comparison function pre-defined for memory symbols.
 *
 * \param [in] symbols Symbol table structure.
 * \param [in] symbol Symbol structure.
 *
 * \return Memory comparison function.  NULL on failure or
 *            no comparison function is specified.
 */
extern bcmdrd_sym_mem_cmp_f
bcmdrd_sym_mem_cmp_fun_get(const bcmdrd_symbols_t *symbols,
                           const bcmdrd_symbol_t *symbol);

/*!
 * \brief Get null entry of a specified memory symbol.
 *
 * The memory null entry is stored in the \c profile member of
 * \ref bcmdrd_symbol_t. This function is used to retrieve the null-entry
 * pre-defined for memory symbols.
 *
 * \param [in] symbols Symbol table structure.
 * \param [in] symbol Symbol structure.
 *
 * \return Memory null entry pointer. NULL on failure or
 *         no null entry is specified.
 */
extern const void *
bcmdrd_sym_mem_null_ent_get(const bcmdrd_symbols_t *symbols,
                            const bcmdrd_symbol_t *symbol);

/*!
 * \brief Get the clear-on-read control for a specified memory symbol.
 *
 * The memory clear-on-read control is stored in the \c profile member of
 * \ref bcmdrd_symbol_t. This function is used to retrieve the clear-on-read
 * control for memory symbols.
 *
 * \param [in] symbols Symbol table structure.
 * \param [in] symbol Symbol structure.
 *
 * \return Pointer to the memory clear-on-read control struct. NULL on failure
 *         or the control is not availabe.
 */
extern const bcmdrd_sym_mem_corctrl_t *
bcmdrd_sym_mem_cor_ctrl_get(const bcmdrd_symbols_t *symbols,
                            const bcmdrd_symbol_t *symbol);

/*!
 * \brief Get reset value and mask of a specified register symbol.
 *
 * This function returns the reset value and reset mask in variable length
 * according to the word-size of the specified register symbol.
 * The first n-word(s) of the return value is the reset value of the specified
 * register which size is n word(s). The following n-word(s) of the
 * return value is the reset mask of the specified register.
 *
 * \param [in] symbols Symbol table structure.
 * \param [in] symbol Symbol structure.
 *
 * \return Pointer to reset value and reset mask or Null on failure.
 */
extern const uint32_t *
bcmdrd_sym_reg_resetval_get(const bcmdrd_symbols_t *symbols,
                            const bcmdrd_symbol_t *symbol);

/*!
 * \brief Check if default value of a specified symbol is non-zero.
 *
 * \param [in] symbols Symbol table structure.
 * \param [in] symbol Symbol structure.
 *
 * \retval true Default value is non-zero.
 * \retval false Default value is zero.
 */
extern bool
bcmdrd_sym_default_value_is_nonzero(const bcmdrd_symbols_t *symbols,
                                    const bcmdrd_symbol_t *symbol);

/*!
 * \brief Get MOR clocks value of a specified symbol for read operation.
 *
 * The MOR (Multiple Outstanding Requests) clocks value is supposed to be
 * used to optimize the CMIC S-bus DMA read opreration performance
 * for this symbol.
 * The returned value from this function can be taken as a reference for the
 * MOR clocks value setting.
 *
 * \param [in] symbols Symbol table structure.
 * \param [in] symbol Symbol structure.
 *
 * \return MOR clocks value or 0 if not supported.
 */
extern uint32_t
bcmdrd_sym_mor_clks_read_get(const bcmdrd_symbols_t *symbols,
                             const bcmdrd_symbol_t *symbol);

/*!
 * \brief Get MOR clocks value of a specified symbol for write operation.
 *
 * The MOR (Multiple Outstanding Requests) clocks value is supposed to be
 * used to optimize the CMIC S-bus DMA write opreration performance
 * for this symbol.
 * The returned value from this function can be taken as a reference for the
 * MOR clocks value setting.
 *
 * \param [in] symbols Symbol table structure.
 * \param [in] symbol Symbol structure.
 *
 * \return MOR clocks value or 0 if not supported.
 */
extern uint32_t
bcmdrd_sym_mor_clks_write_get(const bcmdrd_symbols_t *symbols,
                              const bcmdrd_symbol_t *symbol);

/*!
 * \brief Get device-specific SER data structure.
 *
 * \param [in] symbols Symbol table structure.
 *
 * \return Device-specific SER data structure or NULL if not support.
 */
extern const void *
bcmdrd_sym_ser_data_get(const bcmdrd_symbols_t *symbols);

/*!
 * \brief Get SER profile offset of a specified symbol.
 *
 * Note that the given symbol is indicated to have no SER support if
 * the returned offset is 0.
 *
 * \param [in] symbol Symbol structure.
 *
 * \return Offset to SER profile for a given symbol.
 */
extern int
bcmdrd_sym_ser_profile_offset_get(const bcmdrd_symbol_t *symbol);

#endif /* BCMDRD_SYMBOLS_H */
