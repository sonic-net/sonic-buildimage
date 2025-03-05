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


#ifndef PORTEVENT_MASK_H
#define PORTEVENT_MASK_H

/*------------------------*/
/* Port Event Mask Macros */
/*------------------------*/


/* Number of bytes in mask */
#define PORTEVENT_INDICES   (( LAST_PORT_EVENT - 1) / (sizeof( uchar8) * 8) + 1)

/* Port Event Mask Storage */
typedef struct {
         uchar8 value[PORTEVENT_INDICES];
} PORTEVENT_MASK_t;


typedef PORTEVENT_MASK_t PortEventMask; 
                                        


/*
 * NONZEROMASK returns true if any bit in word mask of NUM length
 * is turned-on.  The result, TRUE or FALSE is stored in
 * result.
 */
#define PORTEVENT_NONZEROMASK(mask, result){                          \
    uint32 _i_;                                                    \
    PORTEVENT_MASK_t *_p_;                                            \
                                                                      \
    _p_ = (PORTEVENT_MASK_t *)&(mask);                                \
    for(_i_ = 0; _i_ < PORTEVENT_INDICES; _i_++)                      \
        if(_p_ -> value[_i_]){                                        \
            result =  TRUE;                                         \
            break;                                                    \
        }                                                             \
        else                                                          \
            result =  FALSE;                                        \
}


#define PORTEVENT_MASKNUMBITSETGET(mask, result){                          \
    uint32 _i_;                                                    \
    PORTEVENT_MASK_t *_p_;                                            \
                                                                      \
    _p_ = (PORTEVENT_MASK_t *)&(mask);result=0;                       \
    for(_i_ = 0; _i_ < PORTEVENT_INDICES; _i_++)                      \
        if(_p_ -> value[_i_]){                                        \
            result++;                                         \
            break;                                                    \
        }                                                             \
}


/* Least significant bit/rightmost bit is lowest port event # */

/* SETMASKBIT turns on bit index # k in mask j. */
#define PORTEVENT_SETMASKBIT(j, k)                                      \
            ((j).value[((k)/(8*sizeof( uchar8)))]                   \
                         |= 1 << ((k) % (8*sizeof( uchar8))))   



/* CLRMASKBIT turns off bit index # k in mask j. */
#define PORTEVENT_CLRMASKBIT(j, k)                                      \
           ((j).value[((k)/(8*sizeof( uchar8)))]                    \
                        &= ~(1 << ((k) % (8*sizeof( uchar8)))))      
            

/* ISMASKBITSET returns 0 if the port event k is not set in mask j */
#define PORTEVENT_ISMASKBITSET(j, k)                                    \
        ((j).value[((k)/(8*sizeof( uchar8)))]                       \
                         & ( 1 << ((k) % (8*sizeof( char8)))) )  

#endif /* PORTEVENT_MASK_H */

