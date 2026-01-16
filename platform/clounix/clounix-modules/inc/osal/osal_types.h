/*******************************************************************************
 *  Copyright Statement:
 *  --------------------
 *  This software and the information contained therein are protected by
 *  copyright and other intellectual property laws and terms herein is
 *  confidential. The software may not be copied and the information
 *  contained herein may not be used or disclosed except with the written
 *  permission of Clounix (Shanghai) Technology Limited. (C) 2020-2023
 *
 *  BY OPENING THIS FILE, BUYER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 *  THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("CLOUNIX SOFTWARE")
 *  RECEIVED FROM CLOUNIX AND/OR ITS REPRESENTATIVES ARE PROVIDED TO BUYER ON
 *  AN "AS-IS" BASIS ONLY. CLOUNIX EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 *  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 *  NEITHER DOES CLOUNIX PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 *  SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 *  SUPPLIED WITH THE CLOUNIX SOFTWARE, AND BUYER AGREES TO LOOK ONLY TO SUCH
 *  THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. CLOUNIX SHALL ALSO
 *  NOT BE RESPONSIBLE FOR ANY CLOUNIX SOFTWARE RELEASES MADE TO BUYER'S
 *  SPECIFICATION OR TO CONFORM TO A PARTICULAR STANDARD OR OPEN FORUM.
 *
 *  BUYER'S SOLE AND EXCLUSIVE REMEDY AND CLOUNIX'S ENTIRE AND CUMULATIVE
 *  LIABILITY WITH RESPECT TO THE CLOUNIX SOFTWARE RELEASED HEREUNDER WILL BE,
 *  AT CLOUNIX'S OPTION, TO REVISE OR REPLACE THE CLOUNIX SOFTWARE AT ISSUE,
 *  OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY BUYER TO
 *  CLOUNIX FOR SUCH CLOUNIX SOFTWARE AT ISSUE.
 *
 *  THE TRANSACTION CONTEMPLATED HEREUNDER SHALL BE CONSTRUED IN ACCORDANCE
 *  WITH THE LAWS OF THE PEOPLE'S REPUBLIC OF CHINA, EXCLUDING ITS CONFLICT OF
 *  LAWS PRINCIPLES.  ANY DISPUTES, CONTROVERSIES OR CLAIMS ARISING THEREOF AND
 *  RELATED THERETO SHALL BE SETTLED BY LAWSUIT IN SHANGHAI,CHINA UNDER.
 *
 *******************************************************************************/

/* FILE NAME:   osal_types.h
 * PURPOSE:
 *      Define the commom data type in CLX SDK.
 * NOTES:
 */

#ifndef OSAL_TYPES_H
#define OSAL_TYPES_H
#ifdef __KERNEL__
#include <linux/types.h>
#else
#include <stdint.h>
#endif

/* INCLUDE FILE DECLARATIONS
 */

/* NAMING CONSTANT DECLARATIONS
 */

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef NULL
#define NULL (void *)0
#endif

#if defined(CLX_EN_HOST_64_BIT_BIG_ENDIAN)
#define UI64_MSW 0
#define UI64_LSW 1
#elif defined(CLX_EN_HOST_64_BIT_LITTLE_ENDIAN)
#define UI64_MSW 1
#define UI64_LSW 0
#else
#define UI64_MSW 1
#define UI64_LSW 0
#endif

#define OSAL_US_PER_SECOND    (1000000) /* macro second per second      */
#define OSAL_NS_PER_USECOND   (1000)    /* nano second per macro second */
#define OSAL_TIME_YEAR_OFFSET (1900)

/* DATA TYPE DECLARATIONS
 */
typedef int BOOL_T;
typedef int8_t I8_T;
typedef uint8_t UI8_T;
typedef int16_t I16_T;
typedef uint16_t UI16_T;
typedef int32_t I32_T;
typedef uint32_t UI32_T;
typedef char C8_T;

#if defined(CLX_EN_COMPILER_SUPPORT_LONG_LONG)
typedef int64_t I64_T;
typedef uint64_t UI64_T;
#else
typedef struct {
    I32_T i64[2];
} I64_T;

typedef struct {
    UI32_T ui64[2];
} UI64_T;
#endif

#if defined(CLX_EN_COMPILER_SUPPORT_LONG_LONG)
#define UI64_HI(dst)                ((UI32_T)((dst) >> 32))
#define UI64_LOW(dst)               ((UI32_T)((dst) & 0xffffffff))
#define UI64_ASSIGN(dst, high, low) ((dst) = (((UI64_T)(high)) << 32 | (UI64_T)(low)))
#define UI64_SET(dst, src)          ((dst) = (src))
#define UI64_ADD_UI32(dst, src)     ((dst) += ((UI64_T)(src)))
#define UI64_SUB_UI32(dst, src)     ((dst) -= ((UI64_T)(src)))
#define UI64_ADD_UI64(dst, src)     ((dst) += (src))
#define UI64_SUB_UI64(dst, src)     ((dst) -= (src))
#define UI64_AND(dst, src)          ((dst) &= (src))
#define UI64_OR(dst, src)           ((dst) |= (src))
#define UI64_XOR(dst, src)          ((dst) ^= (src))
#define UI64_NOT(dst)               ((dst) = ~(dst))
#define UI64_MULT_UI32(dst, src)    ((dst) *= (src))
#define UI64_MULT_UI64(dst, src)    ((dst) *= (src))
/* UI64_T type data comparion:
 * if data1 > data2 return 1
 * if data1 < data2 return -1
 * if data1 == data2 return 0
 */
#define UI64_CMP(data1, data2) (((data1) > (data2)) ? 1 : (((data1) < (data2)) ? -1 : 0))

#define I64_HI(dst)                ((I32_T)((dst) >> 32))
#define I64_LOW(dst)               ((I32_T)((dst) & 0xffffffff))
#define I64_ASSIGN(dst, high, low) ((dst) = (((I64_T)(high)) << 32 | (I64_T)(low)))
#define I64_SET(dst, src)          ((dst) = (src))
#define I64_ADD_UI32(dst, src)     ((dst) += ((I64_T)(src)))
#define I64_SUB_UI32(dst, src)     ((dst) -= ((I64_T)(src)))
#define I64_ADD_UI64(dst, src)     ((dst) += (src))
#define I64_SUB_UI64(dst, src)     ((dst) -= (src))
#define I64_AND(dst, src)          ((dst) &= (src))
#define I64_OR(dst, src)           ((dst) |= (src))
#define I64_XOR(dst, src)          ((dst) ^= (src))
#define I64_NOT(dst)               ((dst) = ~(dst))
#define I64_MULT_UI32(dst, src)    ((dst) *= (src))
#define I64_MULT_UI64(dst, src)    ((dst) *= (src))
/* I64_T type data comparion:
 * if data1 > data2 return 1
 * if data1 < data2 return -1
 * if data1 == data2 return 0
 */
#define I64_CMP(data1, data2) (((data1) > (data2)) ? 1 : (((data1) < (data2)) ? -1 : 0))
#else
#define UI64_HI(dst)  ((dst).ui64[UI64_MSW])
#define UI64_LOW(dst) ((dst).ui64[UI64_LSW])
#define UI64_ASSIGN(dst, high, low) \
    do {                            \
        UI64_HI(dst) = (high);      \
        UI64_LOW(dst) = (low);      \
    } while (0)

#define UI64_SET(dst, src)             \
    do {                               \
        UI64_HI(dst) = UI64_HI(src);   \
        UI64_LOW(dst) = UI64_LOW(src); \
    } while (0)

#define UI64_ADD_UI32(dst, src)     \
    do {                            \
        UI32_T _i_ = UI64_LOW(dst); \
        UI64_LOW(dst) += (src);     \
        if (UI64_LOW(dst) < _i_) {  \
            UI64_HI(dst)++;         \
        }                           \
    } while (0)

#define UI64_SUB_UI32(dst, src)     \
    do {                            \
        UI32_T _i_ = UI64_LOW(dst); \
        UI64_LOW(dst) -= src;       \
        if (UI64_LOW(dst) > _i_) {  \
            UI64_HI(dst)--;         \
        }                           \
    } while (0)

#define UI64_ADD_UI64(dst, src)         \
    do {                                \
        UI32_T _i_ = UI64_LOW(dst);     \
        UI64_LOW(dst) += UI64_LOW(src); \
        if (UI64_LOW(dst) < _i_) {      \
            UI64_HI(dst)++;             \
        }                               \
        UI64_HI(dst) += UI64_HI(src);   \
    } while (0)

#define UI64_SUB_UI64(dst, src)         \
    do {                                \
        UI32_T _i_ = UI64_LOW(dst);     \
        UI64_LOW(dst) -= UI64_LOW(src); \
        if (UI64_LOW(dst) > _i_) {      \
            UI64_HI(dst)--;             \
        }                               \
        UI64_HI(dst) -= UI64_HI(src);   \
    } while (0)

#define UI64_AND(dst, src)              \
    do {                                \
        UI64_HI(dst) &= UI64_HI(src);   \
        UI64_LOW(dst) &= UI64_LOW(src); \
    } while (0)

#define UI64_OR(dst, src)               \
    do {                                \
        UI64_HI(dst) |= UI64_HI(src);   \
        UI64_LOW(dst) |= UI64_LOW(src); \
    } while (0)

#define UI64_XOR(dst, src)              \
    do {                                \
        UI64_HI(dst) ^= UI64_HI(src);   \
        UI64_LOW(dst) ^= UI64_LOW(src); \
    } while (0)

#define UI64_NOT(dst)                   \
    do {                                \
        UI64_HI(dst) = ~UI64_HI(dst);   \
        UI64_LOW(dst) = ~UI64_LOW(dst); \
    } while (0)

/* UI64_T type data comparion:
 * if data1 > data2 return 1
 * if data1 < data2 return -1
 * if data1 == data2 return 0
 */
#define UI64_CMP(data1, data2)                                                     \
    (((data1).ui64[UI64_MSW] > (data2).ui64[UI64_MSW]) ?                           \
         1 :                                                                       \
         (((data1).ui64[UI64_MSW] == (data2).ui64[UI64_MSW]) ?                     \
              (((data1).ui64[UI64_LSW] == (data2).ui64[UI64_LSW]) ?                \
                   0 :                                                             \
                   (((data1).ui64[UI64_LSW] > (data2).ui64[UI64_LSW]) ? 1 : -1)) : \
              -1))

#define UI64_MULT_UI64(dst, src)                              \
    do {                                                      \
        UI32_T _ret_low_ = 0;                                 \
        UI32_T _ret_high_ = 0;                                \
        UI32_T _i_ = 0;                                       \
        UI32_T _j_ = 0;                                       \
        UI32_T _temp_ = 0;                                    \
        UI32_T dst_t[4] = {0, 0, 0, 0};                       \
        UI32_T src_t[4] = {0, 0, 0, 0};                       \
        dst_t[0] = UI64_LOW(dst) & 0xFFFF;                    \
        dst_t[1] = UI64_LOW(dst) >> 16;                       \
        dst_t[2] = UI64_HI(dst) & 0xFFFF;                     \
        dst_t[3] = UI64_HI(dst) >> 16;                        \
        src_t[0] = UI64_LOW(src) & 0xFFFF;                    \
        src_t[1] = UI64_LOW(src) >> 16;                       \
        src_t[2] = UI64_HI(src) & 0xFFFF;                     \
        src_t[3] = UI64_HI(src) >> 16;                        \
        for (_i_ = 0; _i_ < 4; _i_++) {                       \
            for (_j_ = 0; _j_ < 4; _j_++) {                   \
                if ((dst_t[_i_] != 0) && (src_t[_j_] != 0)) { \
                    _temp_ = dst_t[_i_] * src_t[_j_];         \
                    if (0 == (_i_ + _j_)) {                   \
                        _ret_low_ += _temp_;                  \
                        if (_ret_low_ < _temp_) {             \
                            _ret_high_++;                     \
                        }                                     \
                    }                                         \
                    if (1 == (_i_ + _j_)) {                   \
                        _ret_low_ += (_temp_ << 16);          \
                        if (_ret_low_ < (_temp_ << 16)) {     \
                            _ret_high_++;                     \
                        }                                     \
                        _ret_high_ += (_temp_ >> 16);         \
                    }                                         \
                    if (2 == (_i_ + _j_)) {                   \
                        _ret_high_ += _temp_;                 \
                    }                                         \
                    if (3 == (_i_ + _j_)) {                   \
                        _ret_high_ += (_temp_ << 16);         \
                    }                                         \
                }                                             \
            }                                                 \
        }                                                     \
        UI64_HI(dst) = _ret_high_;                            \
        UI64_LOW(dst) = _ret_low_;                            \
    } while (0)

#define UI64_MULT_UI32(dst, src)                              \
    do {                                                      \
        UI32_T _ret_low_ = 0;                                 \
        UI32_T _ret_high_ = 0;                                \
        UI32_T _i_ = 0;                                       \
        UI32_T _j_ = 0;                                       \
        UI32_T _temp_ = 0;                                    \
        UI32_T dst_t[4] = {0, 0, 0, 0};                       \
        UI32_T src_t[2] = {0, 0};                             \
        dst_t[0] = UI64_LOW(dst) & 0xFFFF;                    \
        dst_t[1] = UI64_LOW(dst) >> 16;                       \
        dst_t[2] = UI64_HI(dst) & 0xFFFF;                     \
        dst_t[3] = UI64_HI(dst) >> 16;                        \
        src_t[0] = src & 0xFFFF;                              \
        src_t[1] = src >> 16;                                 \
        for (_i_ = 0; _i_ < 4; _i_++) {                       \
            for (_j_ = 0; _j_ < 2; _j_++) {                   \
                if ((dst_t[_i_] != 0) && (src_t[_j_] != 0)) { \
                    _temp_ = dst_t[_i_] * src_t[_j_];         \
                    if (0 == (_i_ + _j_)) {                   \
                        _ret_low_ += _temp_;                  \
                        if (_ret_low_ < _temp_) {             \
                            _ret_high_++;                     \
                        }                                     \
                    }                                         \
                    if (1 == (_i_ + _j_)) {                   \
                        _ret_low_ += (_temp_ << 16);          \
                        if (_ret_low_ < (_temp_ << 16)) {     \
                            _ret_high_++;                     \
                        }                                     \
                        _ret_high_ += (_temp_ >> 16);         \
                    }                                         \
                    if (2 == (_i_ + _j_)) {                   \
                        _ret_high_ += _temp_;                 \
                    }                                         \
                    if (3 == (_i_ + _j_)) {                   \
                        _ret_high_ += (_temp_ << 16);         \
                    }                                         \
                }                                             \
            }                                                 \
        }                                                     \
        UI64_HI(dst) = _ret_high_;                            \
        UI64_LOW(dst) = _ret_low_;                            \
    } while (0)

#endif

#endif /* OSAL_TYPES_H */
