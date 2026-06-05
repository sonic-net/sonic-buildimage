/* basic_defs.h
 *
 * Driver Framework v4, Basic Definitions.
 *
 * This file provides a number of basic definitions and can be customized for
 * any compiler.
 */

/*****************************************************************************
 * Copyright (c) 2007-2021 by Rambus, Inc. and/or its subsidiaries.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *****************************************************************************/

#ifndef INCLUDE_GUARD_BASIC_DEFS_H
#define INCLUDE_GUARD_BASIC_DEFS_H

#ifndef SWIG
#include <stdint.h>
#endif
/* ============ Compiler-based Configuration ============ */

/*#if __STDC_VERSION__ < 199901L */
/*#error "BasicDefs: C standards older than C99 not supported" */
/*#endif */

#ifdef __cplusplus
#endif

#ifdef _MSC_VER
#define BASICDEFS_COMPILER_MICROSOFT_V1
#define BASICDEFS_TYPES_V1
#define BASICDEFS_BOOL_V1
/* Microsoft compiler only supports "inline" in C++ mode and */
/* expects __inline in C mode */
#define BASICDEFS_INLINE_V1
#endif

#ifdef linux
#ifdef MODULE
/* kernel loadable module */
#define BASICDEFS_COMPILER_LINUX_V1
#define BASICDEFS_INLINE_BUILTIN
#include <linux/types.h>
#else
/* user mode */
#define BASICDEFS_COMPILER_LINUX_V1
#define BASICDEFS_TYPES_STDINT
#define BASICDEFS_BOOL_V1
#define BASICDEFS_INLINE_BUILTIN
#endif
#endif

#ifdef __STDC_VERSION__
#if __STDC_VERSION__ >= 199901L
#define BASICDEFS_TYPES_STDINT
#define BASICDEFS_BOOL_V1
#define BASICDEFS_INLINE_BUILTIN
#endif
#endif

#ifdef __CYGWIN__
#define BASICDEFS_COMPILER_CYGWIN_V1
#define BASICDEFS_TYPES_STDINT
#define BASICDEFS_BOOL_V1
#define BASICDEFS_INLINE_BUILTIN
#endif

/* ============ Basic Types ============ */

#ifdef BASICDEFS_TYPES_STDINT
/* ISO-C99 */
#include <stdint.h>
#endif

#ifdef BASICDEFS_TYPES_V1
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
#endif

/* ============ Boolean type ============ */

#ifdef BASICDEFS_BOOL_V1
/* ISO-C99 */
#include <stdbool.h>
#endif

/* ============ NULL ============ */

#ifndef NULL
#define NULL 0
#endif

/* ============ MIN, MAX ============ */

/* warning for side-effects on the following two macros since the arguments */
/* are evaluated twice changing this to inline functions is problematic */
/* because of type incompatibilities */
#define MIN(_x, _y) ((_x) < (_y) ? (_x) : (_y))
#define MAX(_x, _y) ((_x) > (_y) ? (_x) : (_y))

/* ============ BIT_n ============ */

/* using postfix "U" to be compatible with uint32 */
/* ("UL" is not needed and gives lint warning) */
#define BIT_0  0x00000001U
#define BIT_1  0x00000002U
#define BIT_2  0x00000004U
#define BIT_3  0x00000008U
#define BIT_4  0x00000010U
#define BIT_5  0x00000020U
#define BIT_6  0x00000040U
#define BIT_7  0x00000080U
#define BIT_8  0x00000100U
#define BIT_9  0x00000200U
#define BIT_10 0x00000400U
#define BIT_11 0x00000800U
#define BIT_12 0x00001000U
#define BIT_13 0x00002000U
#define BIT_14 0x00004000U
#define BIT_15 0x00008000U
#define BIT_16 0x00010000U
#define BIT_17 0x00020000U
#define BIT_18 0x00040000U
#define BIT_19 0x00080000U
#define BIT_20 0x00100000U
#define BIT_21 0x00200000U
#define BIT_22 0x00400000U
#define BIT_23 0x00800000U
#define BIT_24 0x01000000U
#define BIT_25 0x02000000U
#define BIT_26 0x04000000U
#define BIT_27 0x08000000U
#define BIT_28 0x10000000U
#define BIT_29 0x20000000U
#define BIT_30 0x40000000U
#define BIT_31 0x80000000U

/* ============ MASK_n_BITS ============ */

#define MASK_1_BIT   (BIT_1 - 1)
#define MASK_2_BITS  (BIT_2 - 1)
#define MASK_3_BITS  (BIT_3 - 1)
#define MASK_4_BITS  (BIT_4 - 1)
#define MASK_5_BITS  (BIT_5 - 1)
#define MASK_6_BITS  (BIT_6 - 1)
#define MASK_7_BITS  (BIT_7 - 1)
#define MASK_8_BITS  (BIT_8 - 1)
#define MASK_9_BITS  (BIT_9 - 1)
#define MASK_10_BITS (BIT_10 - 1)
#define MASK_11_BITS (BIT_11 - 1)
#define MASK_12_BITS (BIT_12 - 1)
#define MASK_13_BITS (BIT_13 - 1)
#define MASK_14_BITS (BIT_14 - 1)
#define MASK_15_BITS (BIT_15 - 1)
#define MASK_16_BITS (BIT_16 - 1)
#define MASK_17_BITS (BIT_17 - 1)
#define MASK_18_BITS (BIT_18 - 1)
#define MASK_19_BITS (BIT_19 - 1)
#define MASK_20_BITS (BIT_20 - 1)
#define MASK_21_BITS (BIT_21 - 1)
#define MASK_22_BITS (BIT_22 - 1)
#define MASK_23_BITS (BIT_23 - 1)
#define MASK_24_BITS (BIT_24 - 1)
#define MASK_25_BITS (BIT_25 - 1)
#define MASK_26_BITS (BIT_26 - 1)
#define MASK_27_BITS (BIT_27 - 1)
#define MASK_28_BITS (BIT_28 - 1)
#define MASK_29_BITS (BIT_29 - 1)
#define MASK_30_BITS (BIT_30 - 1)
#define MASK_31_BITS (BIT_31 - 1)

#define MASK_N_BITS(start, end) ((-1U >> (31 - (end))) & ~((1U << (start)) - 1))

/* ============ EXTRACT_BIT(S) ============ */
#define EXTRACT_BIT(bit, input)         (((input) >> (bit)) & 1U)
#define EXTRACT_BITS(start, end, input) (((input) >> (start)) & MASK_N_BITS(0, (end) - (start)))

/* ============ IDENTIFIER_NOT_USED ============ */

#define IDENTIFIER_NOT_USED(_v) (void)(_v)

/* ============ inline ============ */

#ifdef BASICDEFS_INLINE_V1
#define inline __inline
#endif

#endif /* Inclusion Guard */

/* end of file basic_defs.h */
