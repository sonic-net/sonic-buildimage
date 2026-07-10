// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2017,2021, Pensando Systems Inc.
 */

#include "pciesvc_impl.h"
#include "cfgspace.h"

/*
 * These functions do the actual work of reading/writing
 * the configuration space and associated mask region.
 * These functions understand the implementation details
 * and should not be called directly by external clients.
 *
 * Note that the config space memory region (in cfg->cur[]) is the
 * actual representation of config space for devices exposed across
 * the PCIe bus to the host.  PCIe config space is little-endian.
 * These functions are implemented to be endian-agnostic to run on
 * either big- or little-endian cpus.
 */

static inline u_int8_t
_cfgspace_getb_fld(u_int8_t *fld, const u_int16_t offset)
{
    return fld[offset];
}

static inline u_int16_t
_cfgspace_getw_fld(u_int8_t *fld, const u_int16_t offset)
{
    u_int16_t val;

    val = (((u_int16_t)fld[offset + 1] << 8) |
           ((u_int16_t)fld[offset + 0] << 0));
    return val;
}

static inline u_int32_t
_cfgspace_getd_fld(u_int8_t *fld, const u_int16_t offset)
{
    u_int32_t val;

    val = (((u_int32_t)fld[offset + 3] << 24) |
           ((u_int32_t)fld[offset + 2] << 16) |
           ((u_int32_t)fld[offset + 1] <<  8) |
           ((u_int32_t)fld[offset + 0] <<  0));
    return val;
}

static inline void
_cfgspace_setb_fld(u_int8_t *fld, const u_int16_t offset, const u_int8_t val)
{
    fld[offset] = val;
}

static inline void
_cfgspace_setw_fld(u_int8_t *fld, const u_int16_t offset, const u_int16_t val)
{
    fld[offset + 0] = val;
    fld[offset + 1] = val >> 8;
}

static inline void
_cfgspace_setd_fld(u_int8_t *fld, const u_int16_t offset, const u_int32_t val)
{
    fld[offset + 0] = val;
    fld[offset + 1] = val >> 8;
    fld[offset + 2] = val >> 16;
    fld[offset + 3] = val >> 24;
}

static inline u_int8_t
cfgspace_getb_cur(cfgspace_t *cs, const u_int16_t offset)
{
    return _cfgspace_getb_fld(cs->cur, offset);
}

static inline u_int16_t
cfgspace_getw_cur(cfgspace_t *cs, const u_int16_t offset)
{
    return _cfgspace_getw_fld(cs->cur, offset);
}

static inline u_int32_t
cfgspace_getd_cur(cfgspace_t *cs, const u_int16_t offset)
{
    return _cfgspace_getd_fld(cs->cur, offset);
}

static inline void
cfgspace_setb_cur(cfgspace_t *cs, const u_int16_t offset, const u_int8_t val)
{
    _cfgspace_setb_fld(cs->cur, offset, val);
}

static inline void
cfgspace_setw_cur(cfgspace_t *cs, const u_int16_t offset, const u_int16_t val)
{
    _cfgspace_setw_fld(cs->cur, offset, val);
}

static inline void
cfgspace_setd_cur(cfgspace_t *cs, const u_int16_t offset, const u_int32_t val)
{
    _cfgspace_setd_fld(cs->cur, offset, val);
}

static inline u_int8_t
cfgspace_getb_msk(cfgspace_t *cs, const u_int16_t offset)
{
    return _cfgspace_getb_fld(cs->msk, offset);
}

static inline u_int16_t
cfgspace_getw_msk(cfgspace_t *cs, const u_int16_t offset)
{
    return _cfgspace_getw_fld(cs->msk, offset);
}

static inline u_int32_t
cfgspace_getd_msk(cfgspace_t *cs, const u_int16_t offset)
{
    return _cfgspace_getd_fld(cs->msk, offset);
}

static inline void
cfgspace_setb_msk(cfgspace_t *cs, const u_int16_t offset, const u_int8_t val)
{
    _cfgspace_setb_fld(cs->msk, offset, val);
}

static inline void
cfgspace_setw_msk(cfgspace_t *cs, const u_int16_t offset, const u_int16_t val)
{
    _cfgspace_setw_fld(cs->msk, offset, val);
}

static inline void
cfgspace_setd_msk(cfgspace_t *cs, const u_int16_t offset, const u_int32_t val)
{
    _cfgspace_setd_fld(cs->msk, offset, val);
}

/*****************************************************************/

/*
 * Low-level config space initialization operations.
 */

static u_int8_t
cfgspace_getb(cfgspace_t *cs, u_int16_t offset)
{
    if (offset < cfgspace_size(cs)) {
        return cfgspace_getb_cur(cs, offset);
    }
    /*
     * Any read between end of implementation and
     * end of PCIe Spec size returns 0's.
     */
    if (offset < 4096) {
        return 0;
    }
    /*
     * Shouldn't get any access beyond end of PCIe Spec size,
     * but if so, return all 0xff's.
     */
    return 0xff;
}

static u_int16_t
cfgspace_getw(cfgspace_t *cs, u_int16_t offset)
{
    if (offset + 1 < cfgspace_size(cs)) {
        return cfgspace_getw_cur(cs, offset);
    }
    /*
     * Any read between end of implementation and
     * end of PCIe Spec size returns 0's.
     */
    if (offset + 1 < 4096) {
        return 0;
    }
    /*
     * Shouldn't get any access beyond end of PCIe Spec size,
     * but if so, return all 0xff's.
     */
    return 0xffff;
}

static u_int32_t
cfgspace_getd(cfgspace_t *cs, u_int16_t offset)
{
    if (offset + 3 < cfgspace_size(cs)) {
        return cfgspace_getd_cur(cs, offset);
    }
    /*
     * Any read between end of implementation and
     * end of PCIe Spec size returns 0's.
     */
    if (offset + 3 < 4096) {
        return 0;
    }
    /*
     * Shouldn't get any access beyond end of PCIe Spec size,
     * but if so, return all 0xff's.
     */
    return 0xffffffff;
}

u_int8_t
cfgspace_readb(cfgspace_t *cs, const u_int16_t offset)
{
    return cfgspace_getb(cs, offset);
}

u_int16_t
cfgspace_readw(cfgspace_t *cs, const u_int16_t offset)
{
    return cfgspace_getw(cs, offset);
}

u_int32_t
cfgspace_readd(cfgspace_t *cs, const u_int16_t offset)
{
    return cfgspace_getd(cs, offset);
}

int
cfgspace_read(cfgspace_t *cs,
              const u_int16_t offset,
              const u_int8_t size,
              u_int32_t *valp)
{
    switch (size) {
    case 1: *valp = cfgspace_getb(cs, offset); break;
    case 2: *valp = cfgspace_getw(cs, offset); break;
    case 4: *valp = cfgspace_getd(cs, offset); break;
    default:
        return -1;
    }
    return 0;
}

/*****************************************************************/

/*
 * Config space writes.  Normally config space is initialized
 * with the cfgspace_set* functions.  Once initialized, write
 * accesses come through these APIs where we implement the write-mask
 * fields (initialized with cfgspace_set[bwd]m() functions).
 * A bit set in the write-mask indicates that bit is writeable
 * by these operations.  Read-only fields in the current value are
 * merged with writeable fields from the new written value and the
 * current contents of config space are replaced with the result.
 */

void
cfgspace_writeb(cfgspace_t *cs, const u_int16_t offset, const u_int8_t val)
{
    const u_int8_t oval = cfgspace_getb_cur(cs, offset);
    const u_int8_t wmsk = cfgspace_getb_msk(cs, offset);
    const u_int8_t nval = (oval & ~wmsk) | (val & wmsk);
    cfgspace_setb_cur(cs, offset, nval);
}

void
cfgspace_writew(cfgspace_t *cs, const u_int16_t offset, const u_int16_t val)
{
    const u_int16_t oval = cfgspace_getw_cur(cs, offset);
    const u_int16_t wmsk = cfgspace_getw_msk(cs, offset);
    const u_int16_t nval = (oval & ~wmsk) | (val & wmsk);
    cfgspace_setw_cur(cs, offset, nval);
}

void
cfgspace_writed(cfgspace_t *cs, const u_int16_t offset, const u_int32_t val)
{
    const u_int32_t oval = cfgspace_getd_cur(cs, offset);
    const u_int32_t wmsk = cfgspace_getd_msk(cs, offset);
    const u_int32_t nval = (oval & ~wmsk) | (val & wmsk);
    cfgspace_setd_cur(cs, offset, nval);
}

int
cfgspace_write(cfgspace_t *cs,
               const u_int16_t offset,
               const u_int8_t size,
               const u_int32_t val)
{
    switch (size) {
    case 1: cfgspace_writeb(cs, offset, val); break;
    case 2: cfgspace_writew(cs, offset, val); break;
    case 4: cfgspace_writed(cs, offset, val); break;
    default:
        return -1;
    }
    return 0;
}

/*****************************************************************/

u_int16_t
cfgspace_get_status(cfgspace_t *cs)
{
    return cfgspace_getw(cs, 0x6);
}

u_int8_t
cfgspace_get_headertype(cfgspace_t *cs)
{
    return cfgspace_getb(cs, 0xe);
}

u_int8_t
cfgspace_get_cap(cfgspace_t *cs)
{
    return cfgspace_getb(cs, 0x34);
}

u_int8_t
cfgspace_get_pribus(cfgspace_t *cs)
{
    return cfgspace_getb(cs, 0x18);
}

u_int8_t
cfgspace_get_secbus(cfgspace_t *cs)
{
    return cfgspace_getb(cs, 0x19);
}

u_int8_t
cfgspace_get_subbus(cfgspace_t *cs)
{
    return cfgspace_getb(cs, 0x1a);
}

/*
 * Find capability header with id "capid" in the linked list of
 * capability headers and return the config space address of it.
 * Return 0 if "capid" is not found in the list.
 */
u_int8_t
cfgspace_findcap(cfgspace_t *cs, const u_int8_t capid)
{
    u_int16_t status = cfgspace_get_status(cs);

    /* check Capability List bit in status reg */
    if (status & (1 << 4)) {
        int loops = 256 / 4; /* max-capspace / min-cap-size */
        u_int8_t capaddr;

        for (capaddr = cfgspace_get_cap(cs) & ~0x3;
             loops && capaddr != 0;
             capaddr = cfgspace_getb(cs, capaddr + 0x1) & ~0x3, loops--) {
            const u_int8_t id = cfgspace_getb(cs, capaddr);
            if (id == capid) {
                return capaddr; /* found capid at capaddr */
            }
        }
    }
    return 0; /* not found */
}

/*****************************************************************/

static u_int16_t
extcap_get_id(u_int32_t caphdr)
{
    return caphdr & 0x0000ffff;
}

static u_int16_t
extcap_get_next(u_int32_t caphdr)
{
    return (caphdr >> 20) & 0xffc;
}

/*
 * Find extended capability header with id "capid" in the linked list of
 * extended capability headers and return the config space address of it.
 * Return 0 if "capid" is not found in the list.
 */
u_int16_t
cfgspace_findextcap(cfgspace_t *cs, const u_int16_t capid)
{
    u_int16_t cap;
    u_int32_t caphdr;
    int loops = cfgspace_size(cs) / 4; /* (config size) / (min cap size) */

    cap = 0x100;
    do {
        caphdr = cfgspace_getd(cs, cap);
        if (extcap_get_id(caphdr) == capid) {
            return cap;
        }
        cap = extcap_get_next(caphdr);
    } while (cap && --loops);

    return 0; /* not found */
}
