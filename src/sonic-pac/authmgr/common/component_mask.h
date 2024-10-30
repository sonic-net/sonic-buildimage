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


#ifndef COMPONENT_MASK_H
#define COMPONENT_MASK_H

/*-----------------------*/
/* Component Mask Macros */
/*-----------------------*/

/* Number of bytes in mask */
#define COMPONENT_INDICES   (( LAST_COMPONENT_ID - 1) / (sizeof( uchar8) * 8) + 1)

/* Component Mask Storage */
typedef struct {
         uchar8 value[COMPONENT_INDICES];
} COMPONENT_MASK_t;


typedef COMPONENT_MASK_t ComponentMask; 
                                        
/*
 * NONZEROMASK returns true if any bit in word mask of NUM length
 * is turned-on.  The result, TRUE or FALSE is stored in
 * result.
 */
#define COMPONENT_NONZEROMASK(mask, result){                          \
    uint32 _i_;                                                    \
    COMPONENT_MASK_t *_p_;                                            \
                                                                      \
    _p_ = (COMPONENT_MASK_t *)&(mask);                                \
    for(_i_ = 0; _i_ < COMPONENT_INDICES; _i_++)                      \
        if(_p_ -> value[_i_]){                                        \
            result =  TRUE;                                         \
            break;                                                    \
        }                                                             \
        else                                                          \
            result =  FALSE;                                        \
}


#define COMPONENT_MASKNUMBITSETGET(mask, result){                          \
    uint32 _i_;                                                    \
    COMPONENT_MASK_t *_p_;                                            \
                                                                      \
    _p_ = (COMPONENT_MASK_t *)&(mask);result=0;                       \
    for(_i_ = 0; _i_ < COMPONENT_INDICES; _i_++)                      \
        if(_p_ -> value[_i_]){                                        \
            result++;                                         \
            break;                                                    \
        }                                                             \
}


/* Least significant bit/rightmost bit is lowest interface # */

/* SETMASKBIT turns on bit index # k in mask j. */
#define COMPONENT_SETMASKBIT(j, k)                                      \
            ((j).value[((k-1)/(8*sizeof( uchar8)))]                   \
                         |= 1 << ((k-1) % (8*sizeof( uchar8))))   



/* CLRMASKBIT turns off bit index # k in mask j. */
#define COMPONENT_CLRMASKBIT(j, k)                                      \
           ((j).value[((k-1)/(8*sizeof( uchar8)))]                    \
                        &= ~(1 << ((k-1) % (8*sizeof( uchar8)))))      
            

/* ISMASKBITSET returns 0 if the interface k is not set in mask j */
#define COMPONENT_ISMASKBITSET(j, k)                                    \
        ((j).value[((k-1)/(8*sizeof( uchar8)))]                       \
                         & ( 1 << ((k-1) % (8*sizeof( char8)))) )  





/*--------------------------------------------*/
/*   COMPONENT ACQUISITION MACROS             */
/*
     Used for managing masks of components 
     which have acquired an interface
*/
/*--------------------------------------------*/

/* Number of bytes in mask */
#define COMPONENT_ACQ_INDICES COMPONENT_INDICES} 
#define COMPONENT_ACQUIRED_MASK_t  COMPONENT_MASK_t


typedef COMPONENT_ACQUIRED_MASK_t AcquiredMask;  /*  Mask of components which have 
                                                    "acquired" an interface */

#define COMPONENT_ACQ_NONZEROMASK  COMPONENT_NONZEROMASK 
#define COMPONENT_ACQ_SETMASKBIT  COMPONENT_SETMASKBIT   
#define COMPONENT_ACQ_CLRMASKBIT COMPONENT_CLRMASKBIT
#define COMPONENT_ACQ_ISMASKBITSET  COMPONENT_ISMASKBITSET

  
#endif /* COMPONENT_MASK_H */

