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

/* Copyright (C) 1991-2, RSA Data Security, Inc. Created 1991. All
 * rights reserved.
 *
 * License to copy and use this software is granted provided that it
 * is identified as the "RSA Data Security, Inc. MD5 Message-Digest
 * Algorithm" in all material mentioning or referencing this software
 * or this function.
 *
 * License is also granted to make and use derivative works provided
 * that such works are identified as "derived from the RSA Data
 * Security, Inc. MD5 Message-Digest Algorithm" in all material
 * mentioning or referencing the derived work.
 *
 * RSA Data Security, Inc. makes no representations concerning either
 * the merchantability of this software or the suitability of this
 * software for any particular purpose. It is provided "as is"
 * without express or implied warranty of any kind.
 * These notices must be retained in any copies of any part of this
 * documentation and/or software.
 */

#ifndef MD5_API_H
#define MD5_API_H

#include "pacinfra_common.h"

/* MD5 context. */
typedef struct {
  uint32 state[4];         /* state (ABCD) */
  uint32 count[2];         /* number of bits, modulo 2^64 (lsb first) */
   uchar8 buffer[64];       /* input buffer */
}  MD5_CTX_t;

/* NOTE: In the preceding, 'lsb first' actually means low-order word first.
 *       Each count[] word is stored in host byte order.
 */

/*********************************************************************
* @purpose  Begin a new message digest operation by initializing the context.
*
* @param    *context    @{(input)} Pointer to standard MD5 operating context
*
* @returns  void
*
* @comments Must be called once at the beginning of each new digest 
*           computation.
*
* @end
*********************************************************************/
void md5_init( MD5_CTX_t *context);

/*********************************************************************
* @purpose  Process a message block and update context with latest digest.
*
* @param    *context    @{(input)} Pointer to standard MD5 operating context
* @param    *input      @{(input)} Pointer to message block character array
* @param    inputLen    @{(input)} Message block length, in bytes
*
* @returns  void
*
* @comments May be called repeatedly to process additional parts of the
*           overall message.  The computed message digest is updated 
*           with each invocation of this function.
*
* @end
*********************************************************************/
void md5_update( MD5_CTX_t *context,  uchar8 *input, uint32 inputLen);

/*********************************************************************
* @purpose  Finish the MD5 message-digest calculation and output the result.
*
* @param    *digest     @{(output)} Pointer to message digest output location
* @param    *context    @{(input)}  Pointer to standard MD5 operating context
*
* @returns  void
*
* @comments Must be called after the last MD5 Update operation.
*
* @comments The digest output parameter is intentionally listed first to 
*           match the order specified by RFC 1321.
*
* @end
*********************************************************************/
void md5_final( uchar8 *digest,  MD5_CTX_t *context);

#endif /*MD5_API_H*/

