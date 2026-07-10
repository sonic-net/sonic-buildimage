/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2017-2019,2021, Pensando Systems Inc.
 */

#ifndef __PCIESVC_CFGSPACE_H__
#define __PCIESVC_CFGSPACE_H__

#ifdef __cplusplus
extern "C" {
#if 0
} /* close to calm emacs autoindent */
#endif
#endif

typedef struct cfgspace_s {
    u_int8_t *cur;
    u_int8_t *msk;
    u_int8_t *rst;
    u_int16_t size;
} cfgspace_t;

static inline u_int16_t
cfgspace_size(cfgspace_t *cs)
{
    return cs->size;
}

/* rename these to avoid static link dups */
#define cfgspace_get_status     _pciesvc_cfgspace_get_status
#define cfgspace_get_cap        _pciesvc_cfgspace_get_cap
#define cfgspace_get_pribus     _pciesvc_cfgspace_get_pribus
#define cfgspace_get_secbus     _pciesvc_cfgspace_get_secbus
#define cfgspace_get_subbus     _pciesvc_cfgspace_get_subbus
#define cfgspace_get_headertype _pciesvc_cfgspace_get_headertype
#define cfgspace_findcap        _pciesvc_cfgspace_findcap
#define cfgspace_findextcap     _pciesvc_cfgspace_findextcap
#define cfgspace_readb          _pciesvc_cfgspace_readb
#define cfgspace_readw          _pciesvc_cfgspace_readw
#define cfgspace_readd          _pciesvc_cfgspace_readd
#define cfgspace_read           _pciesvc_cfgspace_read
#define cfgspace_writeb         _pciesvc_cfgspace_writeb
#define cfgspace_writew         _pciesvc_cfgspace_writew
#define cfgspace_writed         _pciesvc_cfgspace_writed
#define cfgspace_write          _pciesvc_cfgspace_write

/*
 * Access specific config space registers.
 */
u_int8_t cfgspace_get_pribus(cfgspace_t *cs);
u_int8_t cfgspace_get_secbus(cfgspace_t *cs);
u_int8_t cfgspace_get_subbus(cfgspace_t *cs);
u_int8_t cfgspace_get_headertype(cfgspace_t *cs);

/*
 * Capabilities.
 */
u_int8_t cfgspace_findcap(cfgspace_t *cs, const u_int8_t capid);

/*
 * Extended Capabilities.
 */
u_int16_t cfgspace_findextcap(cfgspace_t *cs, const u_int16_t capid);

/*
 * Config space operational accessors.
 *
 * Reads return current values, writes apply the write-mask to
 * implement read-only fields.
 */
u_int8_t  cfgspace_readb(cfgspace_t *cs, const u_int16_t offset);
u_int16_t cfgspace_readw(cfgspace_t *cs, const u_int16_t offset);
u_int32_t cfgspace_readd(cfgspace_t *cs, const u_int16_t offset);
int cfgspace_read(cfgspace_t *cs,
                  const u_int16_t offset,
                  const u_int8_t size,
                  u_int32_t *valp);

void cfgspace_writeb(cfgspace_t *cs,
                     const u_int16_t offset, const u_int8_t val);
void cfgspace_writew(cfgspace_t *cs,
                     const u_int16_t offset, const u_int16_t val);
void cfgspace_writed(cfgspace_t *cs,
                     const u_int16_t offset, const u_int32_t val);
int cfgspace_write(cfgspace_t *cs,
                   const u_int16_t offset,
                   const u_int8_t size,
                   const u_int32_t val);

#ifdef __cplusplus
}
#endif

#endif /* __PCIESVC_CFGSPACE_H__ */
