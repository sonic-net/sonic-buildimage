#ifndef __NSS_HAM_SIPHASH24_H
#define __NSS_HAM_SIPHASH24_H
/*
   SipHash reference C implementation

   Written in 2012 by
   Jean-Philippe Aumasson <jeanphilippe.aumasson@gmail.com>
   Daniel J. Bernstein <djb@cr.yp.to>

   To the extent possible under law, the author(s) have dedicated all copyright
   and related and neighboring rights to this software to the public domain
   worldwide. This software is distributed without any warranty.

   You should have received a copy of the CC0 Public Domain Dedication along with
   this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.

   (Minimal changes made by Lennart Poettering, to make clean for inclusion in systemd)
   (Refactored by Tom Gundersen to split up in several functions and follow systemd
    coding style)
*/

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

struct siphash
{
    uint64_t v0;
    uint64_t v1;
    uint64_t v2;
    uint64_t v3;
    uint64_t padding;
    size_t inlen;
};

void siphash24_init(struct siphash *state, const uint8_t k[16]);
void siphash24_compress(const void *in, size_t inlen, struct siphash *state);
#define siphash24_compress_byte(byte, state) siphash24_compress((const uint8_t[]) { (byte) }, 1, (state))

uint64_t siphash24_finalize(struct siphash *state);

uint64_t siphash24_with_key(const void *in, size_t inlen, const uint8_t k[16]);
uint64_t siphash24(const void *in, size_t inlen);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __NSS_HAM_SIPHASH24_H */
