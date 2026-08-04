/* Copyright (C) 2020  MediaTek, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * version 2 along with this program.
 */

/* FILE NAME:   osal_types.h
 * PURPOSE:
 *      Define the commom data type in NPS SDK.
 * NOTES:
 */

#ifndef OSAL_TYPES_H
#define OSAL_TYPES_H

/* INCLUDE FILE DECLARATIONS
 */

/* NAMING CONSTANT DECLARATIONS
 */


#ifndef FALSE
#define FALSE       0
#endif

#ifndef TRUE
#define TRUE        1
#endif

#ifndef NULL
#define NULL        (void *)0
#endif


#if defined(NPS_EN_HOST_64_BIT_BIG_ENDIAN)
#define UI64_MSW 0
#define UI64_LSW 1
#elif defined(NPS_EN_HOST_64_BIT_LITTLE_ENDIAN)
#define UI64_MSW 1
#define UI64_LSW 0
#else
#define UI64_MSW 1
#define UI64_LSW 0
#endif

#if defined(NPS_EN_COMPILER_SUPPORT_LONG_LONG)
#define UI64_HI(dst)                ((UI32_T)((dst) >> 32))
#define UI64_LOW(dst)               ((UI32_T)((dst) & 0xffffffff))
#define UI64_ASSIGN(dst, high, low) ((dst) = ((long long)(high) << 32 | (long long)(low)))
#define UI64_SET(dst, src)          ((dst) = (src))
#define UI64_ADD_UI32(dst, src)     ((dst) += ((long long)(src)))
#define UI64_SUB_UI32(dst, src)     ((dst) -= ((long long)(src)))
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
#define UI64_CMP(data1, data2)       (((data1) > (data2)) ? 1 : (((data1) < (data2)) ? -1 : 0))

#else
#define UI64_HI(dst)                ((dst).ui64[UI64_MSW])
#define UI64_LOW(dst)               ((dst).ui64[UI64_LSW])
#define UI64_ASSIGN(dst, high, low)     \
    do                                  \
    {                                   \
        UI64_HI(dst) = (high);          \
        UI64_LOW(dst) = (low);          \
    } while(0)

#define UI64_SET(dst, src)              \
    do                                  \
    {                                   \
        UI64_HI(dst) = UI64_HI(src);    \
        UI64_LOW(dst) = UI64_LOW(src);  \
    } while(0)


#define UI64_ADD_UI32(dst, src)         \
    do                                  \
    {                                   \
        UI32_T _i_ = UI64_LOW(dst);     \
        UI64_LOW(dst) += (src);         \
        if (UI64_LOW(dst) < _i_)        \
        {                               \
            UI64_HI(dst)++;             \
        }                               \
    } while(0)

#define UI64_SUB_UI32(dst, src)         \
    do                                  \
    {                                   \
        UI32_T _i_ = UI64_LOW(dst);     \
        UI64_LOW(dst) -= src;           \
        if (UI64_LOW(dst) > _i_)        \
        {                               \
            UI64_HI(dst)--;             \
        }                               \
    } while(0)


#define UI64_ADD_UI64(dst, src)         \
    do                                  \
    {                                   \
        UI32_T _i_ = UI64_LOW(dst);     \
        UI64_LOW(dst) += UI64_LOW(src); \
        if (UI64_LOW(dst) < _i_)        \
        {                               \
            UI64_HI(dst)++;             \
        }                               \
        UI64_HI(dst) += UI64_HI(src);   \
    } while(0)

#define UI64_SUB_UI64(dst, src)         \
    do {                                \
        UI32_T _i_ = UI64_LOW(dst);     \
        UI64_LOW(dst) -= UI64_LOW(src); \
        if (UI64_LOW(dst) > _i_)        \
        {                               \
            UI64_HI(dst)--;             \
        }                               \
        UI64_HI(dst) -= UI64_HI(src);   \
    } while(0)


#define UI64_AND(dst, src)              \
    do {                                \
        UI64_HI(dst) &= UI64_HI(src);   \
        UI64_LOW(dst) &= UI64_LOW(src); \
    } while(0)

#define UI64_OR(dst, src)               \
    do {                                \
        UI64_HI(dst) |= UI64_HI(src);   \
        UI64_LOW(dst) |= UI64_LOW(src); \
    } while(0)

#define UI64_XOR(dst, src)              \
    do {                                \
        UI64_HI(dst) ^= UI64_HI(src);   \
        UI64_LOW(dst) ^= UI64_LOW(src); \
    } while(0)

#define UI64_NOT(dst)                   \
    do {                                \
        UI64_HI(dst) = ~UI64_HI(dst);   \
        UI64_LOW(dst) = ~UI64_LOW(dst); \
    } while(0)

/* UI64_T type data comparion:
 * if data1 > data2 return 1
 * if data1 < data2 return -1
 * if data1 == data2 return 0
 */
#define UI64_CMP(data1, data2)                                      \
        (((data1).ui64[UI64_MSW] > (data2).ui64[UI64_MSW])          \
        ? 1 : (((data1).ui64[UI64_MSW] == (data2).ui64[UI64_MSW])   \
        ? (((data1).ui64[UI64_LSW] == (data2).ui64[UI64_LSW])       \
        ? 0 :(((data1).ui64[UI64_LSW] > (data2).ui64[UI64_LSW])     \
        ? 1 : -1)) : -1))

#define UI64_MULT_UI64(dst, src)                                    \
        do                                                          \
        {                                                           \
            UI32_T _ret_low_ = 0;                                   \
            UI32_T _ret_high_ = 0;                                  \
            UI32_T _i_ = 0;                                         \
            UI32_T _j_ = 0;                                         \
            UI32_T _temp_ = 0;                                      \
            UI32_T dst_t[4] = {0, 0, 0, 0};                         \
            UI32_T src_t[4] = {0, 0, 0, 0};                         \
            dst_t[0] = UI64_LOW(dst) & 0xFFFF;                      \
            dst_t[1] = UI64_LOW(dst) >> 16;                         \
            dst_t[2] = UI64_HI(dst) & 0xFFFF;                       \
            dst_t[3] = UI64_HI(dst) >> 16;                          \
            src_t[0] = UI64_LOW(src) & 0xFFFF;                      \
            src_t[1] = UI64_LOW(src) >> 16;                         \
            src_t[2] = UI64_HI(src) & 0xFFFF;                       \
            src_t[3] = UI64_HI(src) >> 16;                          \
            for(_i_ = 0; _i_ < 4; _i_++)                            \
            {                                                       \
                for(_j_ = 0; _j_ < 4; _j_++)                        \
                {                                                   \
                    if((dst_t[_i_] != 0) && (src_t[_j_] != 0))      \
                    {                                               \
                        _temp_ = dst_t[_i_] * src_t[_j_];           \
                        if(0 == (_i_ + _j_))                        \
                        {                                           \
                            _ret_low_ += _temp_;                    \
                            if (_ret_low_ < _temp_)                 \
                            {                                       \
                                _ret_high_++;                       \
                            }                                       \
                        }                                           \
                        if(1 == (_i_ + _j_))                        \
                        {                                           \
                            _ret_low_ += (_temp_ << 16);            \
                            if (_ret_low_ < (_temp_ << 16))         \
                            {                                       \
                                _ret_high_++;                       \
                            }                                       \
                            _ret_high_ += (_temp_ >> 16);           \
                        }                                           \
                        if(2 == (_i_+_j_))                          \
                        {                                           \
                            _ret_high_ += _temp_;                   \
                        }                                           \
                        if(3 == (_i_ + _j_))                        \
                        {                                           \
                            _ret_high_ += (_temp_ << 16);           \
                        }                                           \
                    }                                               \
                }                                                   \
            }                                                       \
            UI64_HI(dst) = _ret_high_;                              \
            UI64_LOW(dst) = _ret_low_;                              \
        } while(0)

#define UI64_MULT_UI32(dst, src)                                    \
        do                                                          \
        {                                                           \
            UI32_T _ret_low_ = 0;                                   \
            UI32_T _ret_high_ = 0;                                  \
            UI32_T _i_ = 0;                                         \
            UI32_T _j_ = 0;                                         \
            UI32_T _temp_ = 0;                                      \
            UI32_T dst_t[4] = {0, 0, 0, 0};                         \
            UI32_T src_t[2] = {0, 0};                               \
            dst_t[0] = UI64_LOW(dst) & 0xFFFF;                      \
            dst_t[1] = UI64_LOW(dst) >> 16;                         \
            dst_t[2] = UI64_HI(dst) & 0xFFFF;                       \
            dst_t[3] = UI64_HI(dst) >> 16;                          \
            src_t[0] = src & 0xFFFF;                                \
            src_t[1] = src >> 16;                                   \
            for(_i_ = 0; _i_ < 4; _i_++)                            \
            {                                                       \
                for(_j_ = 0; _j_ < 2; _j_++)                        \
                {                                                   \
                    if((dst_t[_i_] != 0) && (src_t[_j_] != 0))      \
                    {                                               \
                        _temp_ = dst_t[_i_] * src_t[_j_];           \
                        if(0 == (_i_ + _j_))                        \
                        {                                           \
                            _ret_low_ += _temp_;                    \
                            if (_ret_low_ < _temp_)                 \
                            {                                       \
                                _ret_high_++;                       \
                            }                                       \
                        }                                           \
                        if(1 == (_i_ + _j_))                        \
                        {                                           \
                            _ret_low_ += (_temp_ << 16);            \
                            if (_ret_low_ < (_temp_ << 16))         \
                            {                                       \
                                _ret_high_++;                       \
                            }                                       \
                            _ret_high_ += (_temp_ >> 16);           \
                        }                                           \
                        if(2 == (_i_ + _j_))                        \
                        {                                           \
                            _ret_high_ += _temp_;                   \
                        }                                           \
                        if(3 == (_i_ + _j_))                        \
                        {                                           \
                            _ret_high_ += (_temp_ << 16);           \
                        }                                           \
                    }                                               \
                }                                                   \
            }                                                       \
            UI64_HI(dst) = _ret_high_;                              \
            UI64_LOW(dst) = _ret_low_;                              \
        } while(0)

#endif

/* DATA TYPE DECLARATIONS
 */
typedef int                 BOOL_T;
typedef signed char         I8_T;
typedef unsigned char       UI8_T;
typedef signed short        I16_T;
typedef unsigned short      UI16_T;
typedef signed int          I32_T;
typedef unsigned int        UI32_T;
typedef char                C8_T;

#if defined(NPS_EN_COMPILER_SUPPORT_LONG_LONG)
typedef signed long long int    I64_T;
typedef unsigned long long int  UI64_T;
#else
typedef struct
{
    I32_T   i64[2];
} I64_T;

typedef struct
{
    UI32_T   ui64[2];
} UI64_T;
#endif

#endif /* OSAL_TYPES_H */
