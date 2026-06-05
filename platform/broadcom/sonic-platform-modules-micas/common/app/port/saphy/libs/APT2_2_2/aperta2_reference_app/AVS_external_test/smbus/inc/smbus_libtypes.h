/*
 *
 * $Copyright: (c) 2024 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.$
 *
*/

#ifndef TYPES_H
#define TYPES_H

/*****************************************************************************
*
*               Basic Types
*
*****************************************************************************/

typedef signed   char   S8;
typedef unsigned char   U8;
typedef signed   short  S16;
typedef unsigned short  U16;


#if defined(__unix__) || defined(__arm) || defined(ALPHA) || defined(__PPC__) || defined(__ppc)

    typedef signed   int   S32;
    typedef unsigned int   U32;

#else

    typedef signed   long  S32;
    typedef unsigned long  U32;

#endif


typedef struct _S64
{
    U32          Low;
    S32          High;
} S64;

typedef struct _U64
{
    U32          Low;
    U32          High;
} U64;


/*****************************************************************************
*
*               Pointer Types
*
*****************************************************************************/

typedef S8      *PS8;
typedef U8      *PU8;
typedef S16     *PS16;
typedef U16     *PU16;
typedef S32     *PS32;
typedef U32     *PU32;
typedef S64     *PS64;
typedef U64     *PU64;

#if defined(OS_WINDOWS)
    #define PRIU32 "%lu"
    #define PRIS32 "%ld"
    #define PRIHS32 "%lx"
    #define PRINTU32 "lu"
    #define PRINTS32 "ld"
    #define PRINTHS32 "lx"
    #define PRINTHS "08lX"
#elif defined(OS_LINUX)
    #define PRIU32 "%u"
    #define PRIS32 "%d"
    #define PRIHS32 "%x"
    #define PRINTU32 "u"
    #define PRINTS32 "d"
    #define PRINTHS32 "x"
    #define PRINTHS "08X"
#endif

#endif

