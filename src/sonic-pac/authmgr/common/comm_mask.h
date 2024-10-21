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


#ifndef INCLUDE_COMM_MASK
#define INCLUDE_COMM_MASK

/* USE C Declarations */
#ifdef __cplusplus
extern "C" {
#endif

//#include "default_cnfgr.h" // shiva check with amit
#include "auth_mgr_common.h"
#include "pacinfra_common.h"


/*--------------------------------------*/
/*  Common Mask Macros & Defines        */
/*  Generic Mask Macros & Defines        */
/*--------------------------------------*/

/*
 * Note that these macros are for one-based masks, not zero-based.
 */

typedef  uchar8  mask_values_t;

/* Number of entries per mask unit */
#define  MASK_UNIT                    (sizeof( mask_values_t) * 8)

/*
 *
 * Number of elements of  mask_values_t in mask.
 *
 * Declare a mask using this macro where _size is the number of bits to be
 * used, starting with bit 1.
 *
 * E.g., for a mask for interfaces:
 *
 *  mask_values_t myMask[ MASK_LEN( MAX_INTERFACE_COUNT)];
 *
 */
#define  MASK_LEN(_size)              ((((_size) - 1) /  MASK_UNIT) + 1)

#define  MASK_BYTE_NUM(j)             (sizeof((j)) / sizeof((j)[0]))

/*
 * NONZEROMASK returns 1 if any bit in word mask of NUM length
 * is turned-on.  The result, 1 or 0 is stored in result.
 */
#define  NONZEROMASK(_mask, _result, _size)             \
  do                                                      \
  {                                                       \
    uint32 _i_;                                        \
     mask_values_t *_p_ = ( mask_values_t *)&(_mask); \
                                                          \
    (_result) = 0;                                        \
    for (_i_ = 0; _i_ <  MASK_LEN(_size); _i_++)        \
    {                                                     \
      if (_p_[_i_] != 0)                                  \
      {                                                   \
        (_result) = 1;                                    \
        break;                                            \
      }                                                   \
    }                                                     \
  } while (0)


/* SETMASKBIT turns on bit index # k in mask j. Note: k is one-based. */
#define  SETMASKBIT(j, k)                                          \
  ((j)[((k) - 1) /  MASK_UNIT] |= 1 << (((k) - 1) %  MASK_UNIT))

/* CLRMASKBIT turns off bit index # k in mask j. Note: k is one-based. */
#define  CLRMASKBIT(j, k)                                          \
  ((j)[((k) - 1) /  MASK_UNIT] &= ~(1 << (((k)-1) %  MASK_UNIT)))

/* SETMASKBITVAL sets bit index # k in mask j to value.  Any non-zero value is
 * converted to 1. Note: k is one-based. 
 */
#define  SETMASKBITVAL(j, k, v)                                        \
  ((j)[((k) - 1) /  MASK_UNIT] =                                       \
   (((j)[((k) - 1) /  MASK_UNIT] & ~(1 << (((k)-1) %  MASK_UNIT))) | \
    (!!(v) << (((k) - 1) %  MASK_UNIT))))



/* MASKEQ sets mask j equal to mask k. */
#define  MASKEQ(j, k, _size)                                       \
  do                                                                 \
  {                                                                  \
    uint32 x;                                                     \
                                                                     \
    for (x = 0; x <  MASK_LEN(_size); x++)                         \
    {                                                                \
      (j)[x] = (k)[x];                                               \
    }                                                                \
  } while (0)

/* MASKOREQ or's on the bits in mask j that are on in either mask j or k. */
#define  MASKOREQ(j, k, _size)                                     \
  do                                                                 \
  {                                                                  \
    uint32 x;                                                     \
                                                                     \
    for (x = 0; x <  MASK_LEN(_size); x++)                         \
    {                                                                \
      (j)[x] |= (k)[x];                                              \
    }                                                                \
  } while (0)

/* MASKOR or's two masks on per byte basis */
#define  MASKOR(j, k)                                              \
  do                                                                 \
  {                                                                  \
    uint32 x;                                                     \
                                                                     \
    for (x = 0; x <  MASK_BYTE_NUM(j); x++)                       \
    {                                                                \
      (j)[x] |= (k)[x];                                              \
    }                                                                \
  } while (0)


/* MASKEXOREQ turns-on the bits in mask j that are on in either mask j and k but not in both. */
#define  MASKEXOREQ(j, k, _size)                                   \
  do                                                                 \
  {                                                                  \
    uint32 x;                                                     \
                                                                     \
    for (x = 0; x <  MASK_LEN(_size); x++)                         \
    {                                                                \
      (j)[x] ^= (k)[x];                                              \
    }                                                                \
  } while (0)

/* MASKANDEQ turns-on the bits in mask j that are on in both mask j and k. */
#define  MASKANDEQ(j, k, _size)                                    \
  do                                                                 \
  {                                                                  \
    uint32 x;                                                     \
                                                                     \
    for (x = 0; x <  MASK_LEN(_size); x++)                         \
    {                                                                \
      (j)[x] &= (k)[x];                                              \
    }                                                                \
  } while (0)

/* MASKAND turns-on the bits in mask j that are on in both mask j and k. */
#define  MASKAND(j, k)                                             \
  do                                                                 \
  {                                                                  \
    uint32 x;                                                     \
                                                                     \
    for (x = 0; x <  MASK_BYTE_NUM(j); x++)                        \
    {                                                                \
      (j)[x] &= (k)[x];                                              \
    }                                                                \
  } while (0)

/* MASKINV inverts the bits in mask j. */
#define  MASKINV(j, _size)                                         \
  do                                                                 \
  {                                                                  \
    uint32 x;                                                     \
                                                                     \
    for (x = 0; x <  MASK_LEN(_size); x++)                         \
    {                                                                \
      (j)[x] = ~((j)[x]);                                            \
    }                                                                \
  } while (0)

/* MASKANDEQINV turns on the bits in mask j that are on in both mask j
   and the bitwise-inverse of mask k. */
#define  MASKANDEQINV(j, k, _size)                                 \
  do                                                                 \
  {                                                                  \
    uint32 x;                                                     \
                                                                     \
    for (x = 0; x <  MASK_LEN(_size); x++)                         \
    {                                                                \
      (j)[x] &= ~((k)[x]);                                           \
    }                                                                \
  } while (0)

/* MASKBITSCLR clears bits in mask j that are on in mask k */
#define  MASKBITSCLR(j, k)                                         \
  do                                                                 \
  {                                                                  \
    uint32 x;                                                     \
                                                                     \
    for (x = 0; x <  MASK_BYTE_NUM((j)); x++)                      \
    {                                                                \
      (j)[x] &= ~((k)[x]);                                           \
    }                                                                \
  } while (0)

/* FHMASKBIT finds the index of the most-significant bit turned-on in
   mask j and returns that index in k.  Since this is a 1-based
   mask, 0 is returned for "no bits set". */
#define  FHMASKBIT(j, k, _size)                                    \
  do                                                                 \
  {                                                                  \
     int32 x;                                                      \
                                                                     \
    for (x = ( MASK_LEN(_size) - 1); x >= 0; x--)                  \
    {                                                                \
      if ( (j)[x] )                                                  \
      {                                                              \
        break;                                                       \
      }                                                              \
    }                                                                \
                                                                     \
    k = 0;                                                           \
    if (x >= 0)                                                      \
    {                                                                \
       int32 i;                                                    \
      for (i =  MASK_UNIT - 1; i >= 0; i--)                        \
      {                                                              \
        if ( (j)[x] & (1 << i))                                      \
        {                                                            \
          k = i + 1 + (x *  MASK_UNIT);                            \
          break;                                                     \
        }                                                            \
      }                                                              \
    }                                                                \
  } while (0)

/* ISMASKBITSET returns 0 if the interface k is not set in mask j */
#define  ISMASKBITSET(j, k)                                        \
  ((j)[(((k) - 1) /  MASK_UNIT)] & ( 1 << ((k-1) %  MASK_UNIT)))

/* MASKREV reverses the bits in within each byte of mask j. */
#define  MASKREV(j, _size)                     \
  do                                             \
  {                                              \
    uint32 x,y;                               \
     uchar8 b=0;                               \
                                                 \
    for (x = 0; x <  MASK_LEN(_size); x++)     \
    {                                            \
      b = 0;                                     \
      for (y=0; y<8; y++)                        \
      {                                          \
        if ((j).value[x] & (0x80 >> y))          \
        {                                        \
          b |= 0x1 << y;                         \
        }                                        \
      }                                          \
      (j).value[x] = b;                          \
    }                                            \
  } while (0)

/* FLCLEARMASKBIT finds the index of the least-significant bit turned-off in
 * mask _mask and returns that index in _result.  Since this is a 1-based
 * mask, 0 is returned for "no bits set". */
#define  FLCLEARMASKBIT(_mask, _result, _size)                     \
    do                                                               \
    {                                                                \
         int32 x;                                                  \
         mask_values_t *_p_ = ( mask_values_t *)&(_mask);        \
                                                                     \
        for (x = 0; x <  MASK_LEN(_size); x++)                     \
        {                                                            \
            if ( (_p_)[x]  != 0xFF)                                  \
            {                                                        \
                break;                                               \
            }                                                        \
        }                                                            \
                                                                     \
        if(x <  MASK_LEN(_size))                                   \
        {                                                            \
             int32 i;                                              \
            for (i = 0; i <  MASK_UNIT; i++)                       \
            {                                                        \
                if ( ((_p_)[x] & (1 << i)) == 0)                     \
                {                                                    \
                    _result = i + 1 + (x *  MASK_UNIT);            \
                    break;                                           \
                }                                                    \
            }                                                        \
        }                                                            \
        else                                                         \
        {                                                            \
            _result = 0;                                             \
        }                                                            \
    } while (0)

/* FSCMASKBIT finds the number of bits turned-on in mask _mask and
 *    returns that count in _result. */
#define  FSCMASKBIT(_mask, _result, _size)                         \
    do                                                               \
    {                                                                \
         int32 x;                                                  \
         int32 i;                                                  \
         mask_values_t *_p_ = ( mask_values_t *)&(_mask);        \
                                                                     \
        _result = 0;                                                 \
        for (x = 0; x <  MASK_LEN(_size); x++)                     \
        {                                                            \
            if ( (_p_)[x]  != 0)                                     \
            {                                                        \
                for (i = 0; i <  MASK_UNIT; i++)                   \
                {                                                    \
                    if ( ((_p_)[x] & (1 << i)) != 0)                 \
                    {                                                \
                        _result++;                                   \
                    }                                                \
                                                }                    \
            }                                                        \
        }                                                            \
                                                                     \
    } while (0)


/*--------------------------------------*/
/*  Interface Mask Macros & Defines     */
/*--------------------------------------*/

/* Number of entries per mask byte */
#define  INTF_MASK_UNIT               (sizeof( uchar8) * 8)

/* Number of bytes in mask */
#define  INTF_INDICES   (( MAX_INTERFACE_COUNT - 1) /  INTF_MASK_UNIT + 1)

/* Interface storage */
typedef struct
{
   uchar8   value[ INTF_INDICES];
}  INTF_MASK_t;


/*
 * NONZEROMASK returns 1 if any bit in word mask of NUM length
 * is turned-on.  The result, 1 or 0 is stored in result.
 */
#define  INTF_NONZEROMASK(mask, result){                           \
    uint32 _i_;                                                   \
     INTF_MASK_t *_p_;                                             \
                                                                     \
    _p_ = ( INTF_MASK_t *)&mask;                                   \
    for(_i_ = 0; _i_ <  INTF_INDICES; _i_++)                       \
        if(_p_->value[_i_] != 0){                                    \
            result = 1;                                              \
            break;                                                   \
        }                                                            \
        else                                                         \
            result = 0;                                              \
}


/* Least significant bit/rightmost bit is lowest interface # */
/* this is opposite of what SNMP wants */

/* SETMASKBIT turns on bit index # k in mask j. */
#define  INTF_SETMASKBIT(j, k)                                     \
            ((j).value[((k-1)/(8*sizeof( uchar8)))]                \
                         |= 1 << ((k-1) % (8*sizeof( uchar8))))
                         

/* CLRMASKBIT turns off bit index # k in mask j. */
#define  INTF_CLRMASKBIT(j, k)                                     \
           ((j).value[((k-1)/(8*sizeof( uchar8)))]                 \
                        &= ~(1 << ((k-1) % (8*sizeof( uchar8)))))
                        

/* MASKEQ sets mask j equal to mask k. */
#define  INTF_MASKEQ(j, k) {                                       \
        uint32 x;                                                 \
                                                                     \
        for (x = 0; x <  INTF_INDICES; x++) {                      \
                (j).value[x] = (k).value[x];                         \
        }                                                            \
}


/* MASKOREQ or's on the bits in mask j that are on in either mask j or k. */
#define  INTF_MASKOREQ(j, k) {                                     \
        uint32 x;                                                 \
                                                                     \
        for (x = 0; x <  INTF_INDICES; x++) {                      \
                (j).value[x] |= (k).value[x];                        \
        }                                                            \
}


/* MASKEXOREQ turns-on the bits in mask j that are on in either mask j and k but not in both. */
#define  INTF_MASKEXOREQ(j, k) {                                   \
        uint32 x;                                                 \
                                                                     \
        for (x = 0; x <  INTF_INDICES; x++) {                      \
                j.value[x] ^= k.value[x];                            \
        }                                                            \
}


/* MASKANDEQ turns-on the bits in mask j that are on in both mask j and k. */
#define  INTF_MASKANDEQ(j, k) {                                    \
        uint32 x;                                                 \
                                                                     \
        for (x = 0; x <  INTF_INDICES; x++) {                      \
                (j).value[x] &= (k).value[x];                            \
        }                                                            \
}


/* MASKINV inverts the bits in mask j. */
#define  INTF_MASKINV(j) {                                         \
        uint32 x;                                                 \
                                                                     \
        for (x = 0; x <  INTF_INDICES; x++) {                      \
                j.value[x] = ~(j.value[x]);                          \
        }                                                            \
}


/* MASKANDEQINV turns on the bits in mask j that are on in both mask j
   and the bitwise-inverse of mask k. */
#define  INTF_MASKANDEQINV(j, k) {                                 \
        uint32 x;                                                 \
                                                                     \
        for (x = 0; x <  INTF_INDICES; x++) {                      \
                (j).value[x] &= ~((k).value[x]);                         \
        }                                                            \
}


/* FHMASKBIT finds the index of the most-significant bit turned-on in
   mask j and returns that index in k.  Since this is a 1-based
   mask, 0 is returned for "no bits set". */
#define  INTF_FHMASKBIT(j, k) {                                    \
         int32 x;                                                  \
                                                                     \
        for (x = ( INTF_INDICES - 1); x >= 0; x--) {               \
                if ( j.value[x] )                                    \
                        break;                                       \
        };                                                           \
        k = 0;                                                       \
        if (x >= 0) {                                                \
/* This is for i960
      asm volatile ("scanbit %1,%0" : "=d"(k) : "d"(j.value[x])); */ \
/* This is not */                                                    \
                 int32 i;                                          \
                for (i = 7; i >= 0; i--) {                           \
                    if ( j.value[x] & (1 << i)) {                    \
                       k = i + 1 + (x * (8 * sizeof( uchar8)));    \
                       break;                                        \
                    }                                                \
                }                                                    \
/* End non-i960 */                                                   \
        };                                                           \
}

/* FLMASKBIT finds the index of the least-significant bit turned-on in
   mask j and returns that index in k.  Since this is a 1-based
   mask, 0 is returned for "no bits set". */
#define  INTF_FLMASKBIT(j, k) {                                    \
         int32 x;                                                  \
                                                                     \
        for (x = 0; x <= ( INTF_INDICES - 1); x++) {               \
                if ( j.value[x] )                                    \
                        break;                                       \
        };                                                           \
        k = 0;                                                       \
        if (x <  INTF_INDICES) {                                   \
/* This is for i960
      asm volatile ("scanbit %1,%0" : "=d"(k) : "d"(j.value[x])); */ \
/* This is not */                                                    \
                 int32 i;                                          \
                for (i = 0; i <= 7; i++) {                           \
                    if ( j.value[x] & (1 << i)) {                    \
                       k = i + 1 + (x * (8 * sizeof( uchar8)));    \
                       break;                                        \
                    }                                                \
                }                                                    \
/* End non-i960 */                                                   \
        };                                                           \
}


/* ISMASKBITSET returns 0 if the interface k is not set in mask j */
#define  INTF_ISMASKBITSET(j, k)                                   \
        ((j).value[((k-1)/(8*sizeof( uchar8)))]                    \
                         & ( 1 << ((k-1) % (8*sizeof( char8)))) )
                         

/* MASKREV reverses the bits in within each byte of mask j. */
#define  INTF_MASKREV(j) {                                         \
        uint32 x,y;                                               \
         uchar8 b=0;                                               \
                                                                     \
        for (x = 0; x <  INTF_INDICES; x++) {                      \
                b = 0;                                               \
                for (y=0; y<8; y++)                                  \
                {                                                    \
                  if ((j).value[x] & (0x80 >> y))                    \
                    b |= 0x1 << y;                                   \
                }                                                    \
                (j).value[x] = b;                                    \
        }                                                            \
}



/* Macro definitions for VLAN MASK operations */
/*
 * NONZEROMASK returns 1 if any bit in word mask of NUM length
 * is turned-on.  The result, 1 or 0 is stored in result.
 */
#define  VLAN_NONZEROMASK(mask, result){                           \
    uint32 _i_;                                                   \
     VLAN_MASK_t *_p_;                                             \
                                                                     \
    _p_ = ( VLAN_MASK_t *)&mask;                                   \
    for(_i_ = 0; _i_ <  VLAN_INDICES; _i_++)                       \
        if(_p_->value[_i_] != 0){                                    \
            result = 1;                                              \
            break;                                                   \
        }                                                            \
        else                                                         \
            result = 0;                                              \
}

#define  VLAN_NONZEROMASK_POINTER(mask, result){                   \
    uint32 _i_;                                                   \
     VLAN_MASK_t *_p_;                                             \
                                                                     \
    _p_ = mask;                                                      \
    for(_i_ = 0; _i_ <  VLAN_INDICES; _i_++)                       \
      if(_p_->value[_i_] != 0){                                      \
          result = 1;                                                \
          break;                                                     \
      }                                                              \
      else                                                           \
          result = 0;                                                \
   }

/* Least significant bit/rightmost bit is lowest interface # */
/* this is opposite of what SNMP wants */

/* SETMASKBIT turns on bit index # k in mask j. */
#define  VLAN_SETMASKBIT(j, k)                                     \
            ((j).value[((k-1)/(8*sizeof( uchar8)))]                \
                         |= 1 << ((k-1) % (8*sizeof( uchar8))))


/* CLRMASKBIT turns off bit index # k in mask j. */
#define  VLAN_CLRMASKBIT(j, k)                                     \
           ((j).value[((k-1)/(8*sizeof( uchar8)))]                 \
                        &= ~(1 << ((k-1) % (8*sizeof( uchar8)))))
#define  VLAN_CLRMASKBIT_POINTER(j, k)                                     \
           ((j)->value[((k-1)/(8*sizeof( uchar8)))]                 \
                        &= ~(1 << ((k-1) % (8*sizeof( uchar8)))))

/* MASKEQ sets mask j equal to mask k. */
#define  VLAN_MASKEQ(j, k) {                                       \
        uint32 x;                                                 \
                                                                     \
        for (x = 0; x <  VLAN_INDICES; x++) {                      \
                (j).value[x] = (k).value[x];                         \
        }                                                            \
}


/* MASKOREQ or's on the bits in mask j that are on in either mask j or k. */
#define  VLAN_MASKOREQ(j, k) {                                     \
        uint32 x;                                                 \
                                                                     \
        for (x = 0; x <  VLAN_INDICES; x++) {                      \
                (j).value[x] |= (k).value[x];                        \
        }                                                            \
}


/* MASKEXOREQ turns-on the bits in mask j that are on in either mask j and k but not in both. */
#define  VLAN_MASKEXOREQ(j, k) {                                   \
        uint32 x;                                                 \
                                                                     \
        for (x = 0; x <  VLAN_INDICES; x++) {                      \
                j.value[x] ^= k.value[x];                            \
        }                                                            \
}


/* MASKANDEQ turns-on the bits in mask j that are on in both mask j and k. */
#define  VLAN_MASKANDEQ(j, k) {                                    \
        uint32 x;                                                 \
                                                                     \
        for (x = 0; x <  VLAN_INDICES; x++) {                      \
                j.value[x] &= k.value[x];                            \
        }                                                            \
}


/* MASKINV inverts the bits in mask j. */
#define  VLAN_MASKINV(j) {                                         \
        uint32 x;                                                 \
                                                                     \
        for (x = 0; x <  VLAN_INDICES; x++) {                      \
                j.value[x] = ~(j.value[x]);                          \
        }                                                            \
}


/* MASKANDEQINV turns on the bits in mask j that are on in both mask j
   and the bitwise-inverse of mask k. */
#define  VLAN_MASKANDEQINV(j, k) {                                 \
        uint32 x;                                                 \
                                                                     \
        for (x = 0; x <  VLAN_INDICES; x++) {                      \
                j.value[x] &= ~(k.value[x]);                         \
        }                                                            \
}
#define  VLAN_MASKANDEQINV_POINTER(j, k) {                                 \
        uint32 x;                                                 \
                                                                     \
        for (x = 0; x <  VLAN_INDICES; x++) {                      \
                j->value[x] &= ~(k.value[x]);                         \
        }                                                            \
}



/* ISMASKBITSET returns 0 if the interface k is not set in mask j */
#define  VLAN_ISMASKBITSET(j, k)                                   \
        ((j).value[((k-1)/(8*sizeof( uchar8)))]                    \
                         & ( 1 << ((k-1) % (8*sizeof( char8)))) )

#define  VLAN_ISMASKBITSET_POINTER(j, k)                                   \
        ((j)->value[((k-1)/(8*sizeof( uchar8)))]                    \
                         & ( 1 << ((k-1) % (8*sizeof( char8)))) )


/* FHMASKBIT finds the index of the most-significant bit turned-on in
   mask j and returns that index in k.  Since this is a 1-based
   mask, 0 is returned for "no bits set". */
#define  VLAN_FHMASKBIT(j, k) {                                    \
         int32 x;                                                  \
                                                                     \
        for (x = ( VLAN_INDICES - 1); x >= 0; x--) {               \
                if ( j.value[x] )                                    \
                        break;                                       \
        };                                                           \
        k = 0;                                                       \
        if (x >= 0) {                                                \
/* This is for i960
      asm volatile ("scanbit %1,%0" : "=d"(k) : "d"(j.value[x])); */ \
/* This is not */                                                    \
                 int32 i;                                          \
                for (i = 7; i >= 0; i--) {                           \
                    if ( j.value[x] & (1 << i)) {                    \
                       k = i + 1 + (x * (8 * sizeof( uchar8)));    \
                       break;                                        \
                    }                                                \
                }                                                    \
/* End non-i960 */                                                   \
        };                                                           \
}

/* FLMASKBIT finds the index of the least-significant bit turned-on in
   mask j and returns that index in k.  Since this is a 1-based
   mask, 0 is returned for "no bits set". */
#define  VLAN_FLMASKBIT(j, k) {                                    \
         int32 x;                                                  \
                                                                     \
        for (x = 0; x <= ( VLAN_INDICES - 1); x++) {               \
                if ( j.value[x] )                                    \
                        break;                                       \
        };                                                           \
        k = 0;                                                       \
        if (x <  VLAN_INDICES) {                                   \
/* This is for i960
      asm volatile ("scanbit %1,%0" : "=d"(k) : "d"(j.value[x])); */ \
/* This is not */                                                    \
                 int32 i;                                          \
                for (i = 0; i <= 7; i++) {                           \
                    if ( j.value[x] & (1 << i)) {                    \
                       k = i + 1 + (x * (8 * sizeof( uchar8)));    \
                       break;                                        \
                    }                                                \
                }                                                    \
/* End non-i960 */                                                   \
        };                                                           \
}

/* USE C Declarations */
#ifdef __cplusplus
}
#endif

#endif
