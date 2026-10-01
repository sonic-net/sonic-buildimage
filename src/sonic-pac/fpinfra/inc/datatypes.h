/*
 * Copyright 2024 Broadcom Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef INCLUDE_DATATYPES
#define INCLUDE_DATATYPES

typedef char  char8;
typedef unsigned char  uchar8;
typedef const char  cchar8;

typedef signed char  int8;
typedef unsigned char  uint8;

typedef short  short16;
typedef unsigned short  ushort16;

typedef int  int32;
typedef unsigned int uint32;

typedef unsigned long long  uint64;
typedef signed long long  int64;

typedef double  double64;
typedef float  float32;

/* These data types correspond to the natural integer size of the CPU.
 *  - Useful for loop indicies, etc.
 *  - Do not use for variables whose memory footprint size must be tightly controlled.
 */
typedef unsigned int  uint;
typedef unsigned long  ulong;

typedef void *  netBufHandle;
typedef void *  netBlockHandle;

/* Represents a 64 bit unsigned integer. */
typedef struct
{
  uint32 high;
  uint32 low;
}  ulong64;

/* Intrinsic buffer descriptor.
 *
 * NOTE: This is intended to be used as a basic data type that can be
 *       passed directly between functions -- keep it small and simple.
 *       To use this as an IN/OUT or OUT value, pass a pointer to the
 *       element so that the called routine can update the 'size' field
 *       with the actual content length being output.
 *
 * NOTE: When setting the 'size' field to indicate the amount of data
 *       contained in the buffer, the conventional usage is to account
 *       for all bytes including any string termination characters
 *       (e.g. strlen()+1).
 */
typedef struct
{
  uint32              size;          /* total buffer size (IN) / content length (OUT) */
   char8               *pstart;       /* ptr to buffer starting location */
}  buffdesc;


typedef enum
{
   FALSE = 0,
   TRUE = 1
}  BOOL;

/* We need #ifdef's so that we can compare #if XXX ==  TRUE_DEF */
#define  FALSE_DEF 0
#define  TRUE_DEF 1

/*--------------------------------------*/
/*  Common Return Codes                 */
/*--------------------------------------*/
typedef enum
{
   SUCCESS = 0,
   FAILURE,
   ERROR,
   NOT_IMPLEMENTED_YET,
   NOT_SUPPORTED,
   NOT_EXIST,
   ALREADY_CONFIGURED,
   TABLE_IS_FULL,
   REQUEST_DENIED,
   ASYNCH_RESPONSE,
   ADDR_INUSE,
   NO_VALUE,
   NO_MEMORY,
   DEPENDENCY_NOT_MET,
   HARDWARE_ERROR,
   IMAGE_IN_USE,
   AP_ALREADY_ADDED,
   NOT_APPLICABLE,
   MISMATCH,
   MAY_LOOSE_MGMT_ACCESS,
   RC_MAX
} RC_t;

#ifdef __cplusplus
typedef RC_t (* FUNCPTR_t) (...);  /* ptr to funct returning  RC */
typedef void (* VOIDFUNCPTR_t) (...); /* ptr to fcn rtrning void  */
#else
typedef RC_t (* FUNCPTR_t) ();     /* ptr to function returning  RC */
typedef void (* VOIDFUNCPTR_t) ();    /* ptr to fcn rtrning void  */
#endif /* _cplusplus */

#ifdef __cplusplus
#define EXT_API extern "C"
#else
#define EXT_API extern
#endif /* _cplusplus */

/* For Linux we need to define FUNCPTR type.
 */
#ifdef __cplusplus
typedef  int32 (*FUNCPTR) (...);
#else
typedef  int32 (*FUNCPTR) ();
#endif /* _cplusplus */

typedef void (* VOIDARGFUNCPTR_t) (void *);

#ifdef  METRO_FLEX_PACKAGE
#ifdef  DOT1AD_PACKAGE
typedef RC_t (*fDot1adSetIntfAdminState) ( int8,  int8,  short16,
                                             uint32);
#endif /*  DOT1AD_PACKAGE */
#endif /*  METRO_FLEX_PACKAGE */

#ifndef  NO_PRINTF_FMT
#define  PRINTF_FMT(x,y) __attribute__((format (printf,x,y)))
#else
#define  PRINTF_FMT(x,y)
#endif

#endif /* INCLUDE_DATATYPES */

