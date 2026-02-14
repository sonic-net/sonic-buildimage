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


/* MD5C.C - RSA Data Security, Inc., MD5 message-digest algorithm */

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
 *
 * These notices must be retained in any copies of any part of this
 * documentation and/or software.
 */

#include <string.h>
#include "md5_api.h"

/* Constants for MD5Transform routine */
#define S11 7
#define S12 12
#define S13 17
#define S14 22
#define S21 5
#define S22 9
#define S23 14
#define S24 20
#define S31 4
#define S32 11
#define S33 16
#define S34 23
#define S41 6
#define S42 10
#define S43 15
#define S44 21

static void md5_encode( uchar8 *output, uint32 *input, uint32 len);
static void md5_transform(uint32 *state,  uchar8 *block);
static void md5_decode(uint32 *output,  uchar8 *input, uint32 len);

static  uchar8 md5_padding_g[64] = {
  0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/* F, G, H and I are basic MD5 functions */
#define F(x, y, z) (((x) & (y)) | ((~x) & (z)))
#define G(x, y, z) (((x) & (z)) | ((y) & (~z)))
#define H(x, y, z) ((x) ^ (y) ^ (z))
#define I(x, y, z) ((y) ^ ((x) | (~z)))

/* ROTATE_LEFT rotates x left n bits */
#define ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32-(n))))

/* FF, GG, HH, and II transformations for rounds 1, 2, 3, and 4.
 * Rotation is separate from addition to prevent recomputation.
 */
#define FF(a, b, c, d, x, s, ac) \
{ \
  (a) += F((b), (c), (d)) + (x) + (uint32)(ac); \
  (a) = ROTATE_LEFT ((a), (s)); \
  (a) += (b); \
}
#define GG(a, b, c, d, x, s, ac) \
{ \
  (a) += G((b), (c), (d)) + (x) + (uint32)(ac); \
  (a) = ROTATE_LEFT ((a), (s)); \
  (a) += (b); \
}
#define HH(a, b, c, d, x, s, ac) \
{ \
  (a) += H((b), (c), (d)) + (x) + (uint32)(ac); \
  (a) = ROTATE_LEFT ((a), (s)); \
  (a) += (b); \
}
#define II(a, b, c, d, x, s, ac) \
{ \
  (a) += I ((b), (c), (d)) + (x) + (uint32)(ac); \
  (a) = ROTATE_LEFT ((a), (s)); \
  (a) += (b); \
}


/*********************************************************************
* @purpose  Begin a new message digest operation by initializing the context.
*
* @param    *context    @{(input)} Pointer to MD5 operating context
*
* @returns  void
*
* @comments Must be called once at the beginning of each new digest 
*           computation.
*
* @end
*********************************************************************/
void md5_init( MD5_CTX_t *context)
{
  /* Initialize bit count (it is an 8-byte counter, with count[0] as the
   * low-order word)
   */
  context->count[0] = context->count[1] = 0;

  /* Load magic initialization constants */
  context->state[0] = 0x67452301U;
  context->state[1] = 0xefcdab89U;
  context->state[2] = 0x98badcfeU;
  context->state[3] = 0x10325476U;
}

/*********************************************************************
* @purpose  Process a message block and update context with latest digest.
*
* @param    *context    @{(input)} Pointer to MD5 operating context
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
void md5_update( MD5_CTX_t *context,  uchar8 *input, uint32 inputLen)
{
  uint32     i, index, partLen;

  /* Compute number of bytes mod 64 */
  index = (uint32)((context->count[0] >> 3) & 0x3F);

  /* Update number of bits */
  if ((context->count[0] += ((uint32)inputLen << 3)) < ((uint32)inputLen << 3))
	  context->count[1]++;
	context->count[1] += ((uint32)inputLen >> 29);

  partLen = 64 - index;

  /* Transform as many times as possible */
  if (inputLen >= partLen)
  {
    memcpy(&context->buffer[index], input, (size_t)partLen);
    md5_transform(context->state, context->buffer);

    for (i = partLen; i + 63 < inputLen; i += 64)
      md5_transform(context->state, &input[i]);

    index = 0;
  }
  else
  {
    i = 0;
  }

  /* Buffer remaining input */
  memcpy(&context->buffer[index], &input[i], (size_t)(inputLen-i));
}

/*********************************************************************
* @purpose  Finish the MD5 message-digest calculation and output the result.
*
* @param    *digest     @{(output)} Pointer to message digest output location
* @param    *context    @{(input)}  Pointer to MD5 operating context
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
void md5_final( uchar8 *digest,  MD5_CTX_t *context)
{
   uchar8     bits[8];
  uint32     index, padLen;

  /* Save number of bits */
  md5_encode(bits, context->count, 8);

  /* Pad out to 56 mod 64 */
  index = (uint32)((context->count[0] >> 3) & 0x3f);
  padLen = (index < 56) ? (56 - index) : (120 - index);
  md5_update(context, md5_padding_g, padLen);

  /* Append length (before padding)
   *
   * NOTE: 'before padding' means the length not including the pad bytes, but
   *       this length information is appended to the end of the message.
   */
  md5_update(context, bits, 8);
  
  /* Store state in digest */
  md5_encode(digest, context->state, 16);

  /* Zeroize sensitive information */
  memset(( uchar8 *)context, 0, sizeof(*context));
}

/*********************************************************************
* @purpose  MD5 basic transformation of state info based on message block.
*
* @param    *state      @{(input)} Pointer to MD5 operating context state info
* @param    *block      @{(input)} Pointer to 64-byte message block
*
* @returns  void
*
* @comments Intended for internal use by the MD5 utility.
*
* @end
*********************************************************************/
static void md5_transform(uint32 *state,  uchar8 *block)
{
  register uint32    a, b, c, d;
  uint32             x[16];

  a = state[0], b = state[1], c = state[2], d = state[3];

  md5_decode(x, block, 64);

  /* Round 1 */
  FF (a, b, c, d, x[ 0], S11, 0xd76aa478); /* 1 */
  FF (d, a, b, c, x[ 1], S12, 0xe8c7b756); /* 2 */
  FF (c, d, a, b, x[ 2], S13, 0x242070db); /* 3 */
  FF (b, c, d, a, x[ 3], S14, 0xc1bdceee); /* 4 */
  FF (a, b, c, d, x[ 4], S11, 0xf57c0faf); /* 5 */
  FF (d, a, b, c, x[ 5], S12, 0x4787c62a); /* 6 */
  FF (c, d, a, b, x[ 6], S13, 0xa8304613); /* 7 */
  FF (b, c, d, a, x[ 7], S14, 0xfd469501); /* 8 */
  FF (a, b, c, d, x[ 8], S11, 0x698098d8); /* 9 */
  FF (d, a, b, c, x[ 9], S12, 0x8b44f7af); /* 10 */
  FF (c, d, a, b, x[10], S13, 0xffff5bb1); /* 11 */
  FF (b, c, d, a, x[11], S14, 0x895cd7be); /* 12 */
  FF (a, b, c, d, x[12], S11, 0x6b901122); /* 13 */
  FF (d, a, b, c, x[13], S12, 0xfd987193); /* 14 */
  FF (c, d, a, b, x[14], S13, 0xa679438e); /* 15 */
  FF (b, c, d, a, x[15], S14, 0x49b40821); /* 16 */

 /* Round 2 */
  GG (a, b, c, d, x[ 1], S21, 0xf61e2562); /* 17 */
  GG (d, a, b, c, x[ 6], S22, 0xc040b340); /* 18 */
  GG (c, d, a, b, x[11], S23, 0x265e5a51); /* 19 */
  GG (b, c, d, a, x[ 0], S24, 0xe9b6c7aa); /* 20 */
  GG (a, b, c, d, x[ 5], S21, 0xd62f105d); /* 21 */
  GG (d, a, b, c, x[10], S22,  0x2441453); /* 22 */
  GG (c, d, a, b, x[15], S23, 0xd8a1e681); /* 23 */
  GG (b, c, d, a, x[ 4], S24, 0xe7d3fbc8); /* 24 */
  GG (a, b, c, d, x[ 9], S21, 0x21e1cde6); /* 25 */
  GG (d, a, b, c, x[14], S22, 0xc33707d6); /* 26 */
  GG (c, d, a, b, x[ 3], S23, 0xf4d50d87); /* 27 */
  GG (b, c, d, a, x[ 8], S24, 0x455a14ed); /* 28 */
  GG (a, b, c, d, x[13], S21, 0xa9e3e905); /* 29 */
  GG (d, a, b, c, x[ 2], S22, 0xfcefa3f8); /* 30 */
  GG (c, d, a, b, x[ 7], S23, 0x676f02d9); /* 31 */
  GG (b, c, d, a, x[12], S24, 0x8d2a4c8a); /* 32 */

  /* Round 3 */
  HH (a, b, c, d, x[ 5], S31, 0xfffa3942); /* 33 */
  HH (d, a, b, c, x[ 8], S32, 0x8771f681); /* 34 */
  HH (c, d, a, b, x[11], S33, 0x6d9d6122); /* 35 */
  HH (b, c, d, a, x[14], S34, 0xfde5380c); /* 36 */
  HH (a, b, c, d, x[ 1], S31, 0xa4beea44); /* 37 */
  HH (d, a, b, c, x[ 4], S32, 0x4bdecfa9); /* 38 */
  HH (c, d, a, b, x[ 7], S33, 0xf6bb4b60); /* 39 */
  HH (b, c, d, a, x[10], S34, 0xbebfbc70); /* 40 */
  HH (a, b, c, d, x[13], S31, 0x289b7ec6); /* 41 */
  HH (d, a, b, c, x[ 0], S32, 0xeaa127fa); /* 42 */
  HH (c, d, a, b, x[ 3], S33, 0xd4ef3085); /* 43 */
  HH (b, c, d, a, x[ 6], S34,  0x4881d05); /* 44 */
  HH (a, b, c, d, x[ 9], S31, 0xd9d4d039); /* 45 */
  HH (d, a, b, c, x[12], S32, 0xe6db99e5); /* 46 */
  HH (c, d, a, b, x[15], S33, 0x1fa27cf8); /* 47 */
  HH (b, c, d, a, x[ 2], S34, 0xc4ac5665); /* 48 */

  /* Round 4 */
  II (a, b, c, d, x[ 0], S41, 0xf4292244); /* 49 */
  II (d, a, b, c, x[ 7], S42, 0x432aff97); /* 50 */
  II (c, d, a, b, x[14], S43, 0xab9423a7); /* 51 */
  II (b, c, d, a, x[ 5], S44, 0xfc93a039); /* 52 */
  II (a, b, c, d, x[12], S41, 0x655b59c3); /* 53 */
  II (d, a, b, c, x[ 3], S42, 0x8f0ccc92); /* 54 */
  II (c, d, a, b, x[10], S43, 0xffeff47d); /* 55 */
  II (b, c, d, a, x[ 1], S44, 0x85845dd1); /* 56 */
  II (a, b, c, d, x[ 8], S41, 0x6fa87e4f); /* 57 */
  II (d, a, b, c, x[15], S42, 0xfe2ce6e0); /* 58 */
  II (c, d, a, b, x[ 6], S43, 0xa3014314); /* 59 */
  II (b, c, d, a, x[13], S44, 0x4e0811a1); /* 60 */
  II (a, b, c, d, x[ 4], S41, 0xf7537e82); /* 61 */
  II (d, a, b, c, x[11], S42, 0xbd3af235); /* 62 */
  II (c, d, a, b, x[ 2], S43, 0x2ad7d2bb); /* 63 */
  II (b, c, d, a, x[ 9], S44, 0xeb86d391); /* 64 */

  state[0] += a;
  state[1] += b;
  state[2] += c;
  state[3] += d;

  /* Zeroize sensitive information */
  memset(( uchar8 *)x, 0, sizeof(x));
}

/*********************************************************************
* @purpose  Converts a set of unsigned integers into a byte stream in 
*           little endian order.
*
* @param    *output     @{(output)} Pointer to character array 
* @param    *input      @{(input)}  Pointer to unsigned integer list
* @param    len         @{(input)}  Input byte count
*
* @returns  void
*
* @comments Intended for internal use by the MD5 utility.
*
* @comments The len parameter is assumed to be a multiple of 4.
*
* @comments This routine preserves endian neutrality for the MD5 algorithm.
*           (On little endian systems, this code is redundant, as a simple
*           casting would produce the same result.)
*
* @end
*********************************************************************/
static void md5_encode( uchar8 *output, uint32 *input, uint32 len)
{
  uint32 i, j;

  for (i = 0, j = 0; j < len; i++, j += 4)
  {
    output[j]   = ( uchar8)(input[i] & 0xff);
    output[j+1] = ( uchar8)((input[i] >> 8) & 0xff);
    output[j+2] = ( uchar8)((input[i] >> 16) & 0xff);
    output[j+3] = ( uchar8)((input[i] >> 24) & 0xff);
  }
}

/*********************************************************************
* @purpose  Converts a byte stream to a set of unsigned integers in 
*           little endian order.
*
* @param    *output     @{(output)} Pointer to unsigned integer list 
* @param    *input      @{(input)}  Pointer to character array
* @param    len         @{(input)}  Input byte count
*
* @returns  void
*
* @comments Intended for internal use by the MD5 utility.
*
* @comments The len parameter is assumed to be a multiple of 4.
*
* @comments This routine preserves endian neutrality for the MD5 algorithm.
*           (On little endian systems, a simple memcpy operation would 
*           would produce the same result.)
*
* @end
*********************************************************************/
static void md5_decode(uint32 *output,  uchar8 *input, uint32 len)
{
  uint32 i, j;

  for (i = 0, j = 0; j < len; i++, j += 4)
    output[i] = ((uint32)input[j]) | (((uint32)input[j+1]) << 8) |
                (((uint32)input[j+2]) << 16) | (((uint32)input[j+3]) << 24);
}



/*@ignore@*/

/*------------------------------------------------------------------------
 * The following code is not part of the actual MD5 utility, but is
 * used to check conformance of the implementation against the algorithm
 * specified by RFC 1321.  As such, it is kept with this file to allow
 * implementation changes to be verified.
 *
 * Run 'devshell MD5TestSuite' for conformance verification.  Optionally,
 * the command 'devshell MD5TimeTrial(n)' can be used to check how long
 * it takes to compute the MD5 message digest for a message consisting
 * of n blocks of fixed size.
 *------------------------------------------------------------------------
 */

#include <stdio.h>
#include <time.h>

void MD5TimeTrial(uint32 blockCount);
void MD5TestSuite(void);
void MD5String_v( uchar8 *string,  uchar8 answer[16]);
void MD5Print( uchar8 digest[16]);

/* Measures the time to digest TEST_BLOCK_COUNT TEST_BLOCK_LEN-byte
  blocks.
 */
void MD5TimeTrial(uint32 blockCount)
{
#define TEST_BLOCK_LEN        1500
#define TEST_BLOCK_COUNT_MAX  10000
   MD5_CTX_t  context;
  time_t        endTime, startTime;
   uchar8     block[TEST_BLOCK_LEN], digest[16];
  uint32     i;
  time_t        deltaTime;

  if (blockCount > TEST_BLOCK_COUNT_MAX)
  {
    printf("Block count too large.  Must be between 0 and %u\n", 
           (uint32)TEST_BLOCK_COUNT_MAX);
    return;
  }

  printf("MD5 time trial. Digesting %d %d-byte blocks ...",
         blockCount, TEST_BLOCK_LEN);

  /* Initialize block */
  for (i = 0; i < TEST_BLOCK_LEN; i++)
    block[i] = ( uchar8)(i & 0xff);

  /* Start timer */
  time(&startTime);

  /* Digest blocks */
  md5_init(&context);
  for (i = 0; i < blockCount; i++)
    md5_update(&context, block, TEST_BLOCK_LEN);
  md5_final(digest, &context);

  /* Stop timer */
  time (&endTime);
  deltaTime = endTime - startTime;

  printf(" done\n");
  printf("Digest = ");
  MD5Print(digest);
  printf("\nTime = %u seconds\n", (uint32)deltaTime);
  if ((uint32)deltaTime > 0)
    printf("Speed = %u bytes/second\n",
           (uint32)(TEST_BLOCK_LEN * blockCount/deltaTime));
}

/* Digests a reference suite of strings and prints and verifies the results.
 */
void MD5TestSuite(void)
{
  unsigned char answer1[16] = 
    {0xd4, 0x1d, 0x8c, 0xd9, 0x8f, 0x00, 0xb2, 0x04, 0xe9, 0x80, 0x09, 0x98, 0xec, 0xf8, 0x42, 0x7e};
  unsigned char answer2[16] = 
    {0x0c, 0xc1, 0x75, 0xb9, 0xc0, 0xf1, 0xb6, 0xa8, 0x31, 0xc3, 0x99, 0xe2, 0x69, 0x77, 0x26, 0x61};
  unsigned char answer3[16] = 
    {0x90, 0x01, 0x50, 0x98, 0x3c, 0xd2, 0x4f, 0xb0, 0xd6, 0x96, 0x3f, 0x7d, 0x28, 0xe1, 0x7f, 0x72};
  unsigned char answer4[16] = 
    {0xf9, 0x6b, 0x69, 0x7d, 0x7c, 0xb7, 0x93, 0x8d, 0x52, 0x5a, 0x2f, 0x31, 0xaa, 0xf1, 0x61, 0xd0};
  unsigned char answer5[16] = 
    {0xc3, 0xfc, 0xd3, 0xd7, 0x61, 0x92, 0xe4, 0x00, 0x7d, 0xfb, 0x49, 0x6c, 0xca, 0x67, 0xe1, 0x3b};
  unsigned char answer6[16] = 
    {0xd1, 0x74, 0xab, 0x98, 0xd2, 0x77, 0xd9, 0xf5, 0xa5, 0x61, 0x1c, 0x2c, 0x9f, 0x41, 0x9d, 0x9f};
  unsigned char answer7[16] = 
    {0x57, 0xed, 0xf4, 0xa2, 0x2b, 0xe3, 0xc9, 0x55, 0xac, 0x49, 0xda, 0x2e, 0x21, 0x07, 0xb6, 0x7a};

  printf ("MD5 test suite:\n");

  MD5String_v ("", answer1);
  MD5String_v ("a", answer2);
  MD5String_v ("abc", answer3);
  MD5String_v ("message digest", answer4);
  MD5String_v ("abcdefghijklmnopqrstuvwxyz", answer5);
  MD5String_v
 ("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789", answer6);
  MD5String_v
 ("1234567890123456789012345678901234567890\
1234567890123456789012345678901234567890", answer7);
}

/* Digests a string and prints the result with verification.
 */
void MD5String_v( uchar8 *string,  uchar8 answer[16])
{
   MD5_CTX_t context;
   uchar8 digest[16];
  uint32 len = strlen(string);

  md5_init(&context);
  md5_update(&context, string, len);
  md5_final(digest, &context);

  printf ("MD5 (\"%s\") = ", string);
  MD5Print (digest);
  printf (" (%4s)\n", (memcmp(digest, answer, 16)==0) ? "pass" : "FAIL");
}

/* Prints a message digest in hexadecimal.
 */
void MD5Print( uchar8 digest[16])
{
  uint32     i;

  for (i = 0; i < 16; i++)
    printf ("%02x", digest[i]);
}

/*@end@*/
