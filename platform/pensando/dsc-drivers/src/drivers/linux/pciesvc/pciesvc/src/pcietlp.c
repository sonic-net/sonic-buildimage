// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2018,2021-2022, Pensando Systems Inc.
 */

#include "pciesvc_impl.h"
#include "pcietlp.h"
#include "bdf.h"

typedef struct pcietlp_info_s {
    unsigned int error:1;
    char error_str[80];
} pcietlp_info_t;

static pcietlp_info_t pcietlp_info;

static inline int
bitcount(u_int32_t n)
{
    int count = 0;

    while (n) {
	count++;
        n &= ~(-n); /* clear low order 1 bit */
    }
    return count;
}

static int pcietlp_set_error(const char *fmt, ...)
    __attribute__((format (printf, 1, 2)));
static int pcietlp_set_error(const char *fmt, ...)
{
    pcietlp_info_t *pi = &pcietlp_info;

    if (pi->error == 0) {
        va_list ap;

        va_start(ap, fmt);
        pciesvc_vsnprintf(pi->error_str, sizeof(pi->error_str), fmt, ap);
        va_end(ap);
        pi->error = 1;
    }
    return -1;
}

static void
pcietlp_clr_error(void)
{
    pcietlp_info_t *pi = &pcietlp_info;
    pi->error_str[0] = '\0';
    pi->error = 0;
}

static int
pcietlp_is_error(void)
{
    pcietlp_info_t *pi = &pcietlp_info;
    return pi->error;
}

char *
pcietlp_get_error(void)
{
    pcietlp_info_t *pi = &pcietlp_info;
    return pi->error_str;
}

static u_int32_t
stlp_dw(const pcie_stlp_t *stlp)
{
    const u_int64_t dw_start = stlp->addr >> 2;
    const u_int64_t dw_end   = (stlp->addr + stlp->size + 3) >> 2;

    return dw_end - dw_start;
}

static u_int32_t
stlp_fbe(const pcie_stlp_t *stlp)
{
    static const u_int8_t betab[4] = { 0xf, 0xe, 0xc, 0x8 };

    if (stlp_dw(stlp) <= 1) {
        u_int8_t fbe = (1 << stlp->size) - 1;
        return fbe << (stlp->addr & 0x3);
    }
    return betab[stlp->addr & 0x3];
}

static u_int32_t
stlp_lbe(const pcie_stlp_t *stlp)
{
    static const u_int8_t betab[4] = { 0xf, 0x8, 0xc, 0xe };

    /* ndw == 1 all encoded in fbe, no lbe bits */
    if (stlp_dw(stlp) <= 1) return 0;

    return betab[(stlp->addr + stlp->size) & 0x3];
}

/******************************************************************/

static void
encode_addr32(const pcie_stlp_t *stlp, u_int32_t *addrp)
{
    addrp[0] = pciesvc_htobe32(stlp->addr & ~0x3); /* DW-align addr */
}

static void
decode_addr32(pcie_stlp_t *stlp, const u_int32_t addr)
{
    stlp->addr += pciesvc_be32toh(addr);
}

/******************************************************************/

static void
encode_addr64(const pcie_stlp_t *stlp, u_int32_t *addrp)
{
    addrp[0] = pciesvc_htobe32(stlp->addr >> 32);
    addrp[1] = pciesvc_htobe32(stlp->addr & ~0x3); /* DW-align addr */
}

static void
decode_addr64(pcie_stlp_t *stlp, const u_int32_t *addrp)
{
    stlp->addr += ((u_int64_t)pciesvc_be32toh(addrp[0]) << 32) |
                              pciesvc_be32toh(addrp[1]);
}

/******************************************************************/

static void
encode_data32(const pcie_stlp_t *stlp, u_int32_t *datap)
{
    u_int32_t v = stlp->data;

    /* shift data over to byte lanes based on addr */
    v <<= (stlp->addr & 0x3) * 8;

    datap[0] = pciesvc_htole32(v);
}

static void
decode_data32(pcie_stlp_t *stlp, const u_int32_t *datap)
{
    const u_int32_t v = pciesvc_le32toh(*datap);

    stlp->data = v >> ((stlp->addr & 0x3) * 8);

    /* mask off unused byte lanes */
    if (stlp->size < 4) {
        const u_int32_t datamask = (1 << stlp->size * 8) - 1;
        stlp->data &= datamask;
    }
}

/******************************************************************/

static void
encode_data64(const pcie_stlp_t *stlp, u_int32_t *datap)
{
    u_int64_t v = stlp->data;

    /* shift data over to byte lanes based on addr */
    v <<= (stlp->addr & 0x3) * 8;

    datap[0] = pciesvc_htole32(v);
    datap[1] = pciesvc_htole32(v >> 32);
}

static void
decode_data64(pcie_stlp_t *stlp, const u_int32_t *datap)
{
    const u_int64_t v = (pciesvc_le32toh(datap[0]) |
                         (u_int64_t)pciesvc_le32toh(datap[1]) << 32);

    stlp->data = v >> ((stlp->addr & 0x3) * 8);

    /* mask off unused byte lanes */
    if (stlp->size < 8) {
        const u_int64_t datamask = (1ULL << stlp->size * 8) - 1;
        stlp->data &= datamask;
    }
}

/******************************************************************/

static void
encode_data(const pcie_stlp_t *stlp, u_int32_t *datap)
{
    if (stlp_dw(stlp) <= 1) {
        encode_data32(stlp, datap);
    } else {
        encode_data64(stlp, datap);
    }
}

static void
decode_data(pcie_stlp_t *stlp, const u_int32_t *datap)
{
    if (stlp_dw(stlp) <= 1) {
        decode_data32(stlp, datap);
    } else {
        decode_data64(stlp, datap);
    }
}

/******************************************************************/

static void
encode_cmn_hdr(const pcie_stlp_t *stlp, const u_int8_t type, void *rtlp)
{
    pcie_tlp_common_hdr_t *hdr = rtlp;
    u_int16_t ndw = stlp_dw(stlp);

    hdr->type = type;

    if (ndw == 0) {
        /* 0-length transaction is allowed, ndw=1 and be=0 */
        ndw = 1;
    } else if (ndw == 0x400) {
        /* 0x400 dw encoded as len=0 */
        ndw = 0;
    } else if (ndw > 0x400) {
        /* can't encode > 0x400 */
        pcietlp_set_error("encode_cmn_hdr: ndw %d > 0x400", ndw);
        return;
    }
    hdr->len_lo = ndw;
    hdr->len_hi = ndw >> 8;
    hdr->reqid = pciesvc_htobe16(stlp->reqid);
    hdr->tag = stlp->tag;
    hdr->t8 = stlp->tag >> 8;
    hdr->t9 = stlp->tag >> 9;
    hdr->fbe = stlp_fbe(stlp);
    hdr->lbe = stlp_lbe(stlp);
}

static void
decode_cmn_hdr(pcie_stlp_t *stlp, const void *rtlp)
{
    const pcie_tlp_common_hdr_t *hdr = rtlp;
    const u_int8_t be_dw = (hdr->fbe > 0) + (hdr->lbe > 0);
    const u_int8_t be_bits = bitcount(hdr->fbe) + bitcount(hdr->lbe);
    const u_int8_t ffbe = pciesvc_ffs(hdr->fbe);
    u_int16_t ndw = (hdr->len_hi << 8) | hdr->len_lo;

    /* ndw=0 indicates max 0x400 */
    if (ndw == 0) ndw = 0x400;

    /* Compute size.  Start with ndw, then adjust for the Byte Enable bits. */
    if (ndw == 1 && !be_bits) {
        stlp->size = 0;
    } else {
        stlp->size = ((ndw - be_dw) << 2) + be_bits;
    }

    /* addr start depends on first First Byte Enable bit position.*/
    stlp->addr = ffbe ? ffbe - 1 : 0;

    stlp->reqid = pciesvc_be16toh(hdr->reqid);
    stlp->tag = (hdr->t9 << 9) | (hdr->t8 << 8) | hdr->tag;
}

/******************************************************************/

static void
encode_cfg_hdr(const pcie_stlp_t *stlp, const u_int8_t type, void *rtlp)
{
    pcie_tlp_cfg_t *cfg = rtlp;

    encode_cmn_hdr(stlp, type, cfg);
    cfg->bdf = pciesvc_htobe16(stlp->bdf);
    cfg->reg = stlp->addr & ~0x3;       /* DW-aligned reg */
    cfg->extreg = stlp->addr >> 8;
}

static void
decode_cfg_hdr(pcie_stlp_t *stlp, const void *rtlp)
{
    const pcie_tlp_cfg_t *cfg = rtlp;

    decode_cmn_hdr(stlp, cfg);
    stlp->bdf = pciesvc_be16toh(cfg->bdf);
    stlp->addr += (cfg->extreg << 8) | cfg->reg;
}

/******************************************************************/

static void
encode_mem32_hdr(const pcie_stlp_t *stlp, const u_int8_t type, void *rtlp)
{
    pcie_tlp_mem32_t *mem = rtlp;

    encode_cmn_hdr(stlp, type, mem);
    encode_addr32(stlp, &mem->addr);
}

static void
decode_mem32_hdr(pcie_stlp_t *stlp, const void *rtlp)
{
    const pcie_tlp_mem32_t *mem = rtlp;

    decode_cmn_hdr(stlp, mem);
    decode_addr32(stlp, mem->addr);
}

/******************************************************************/

static void
encode_mem64_hdr(const pcie_stlp_t *stlp, const u_int8_t type, void *rtlp)
{
    pcie_tlp_mem64_t *mem = rtlp;

    encode_cmn_hdr(stlp, type, mem);
    encode_addr64(stlp, &mem->addr_hi);
}

static void
decode_mem64_hdr(pcie_stlp_t *stlp, const void *rtlp)
{
    const pcie_tlp_mem64_t *mem = rtlp;

    decode_cmn_hdr(stlp, mem);
    decode_addr64(stlp, &mem->addr_hi);
}

/******************************************************************
 * CFG
 */

static int
encode_cfgrd(const pcie_stlp_t *stlp, void *rtlp, const size_t rtlpsz)
{
    const int tlpsz = 12;

    if (rtlpsz < tlpsz) {
        return pcietlp_set_error("cfgrd: rtlpsz want %d got %ld",
                                 tlpsz, rtlpsz);
    }
    if (stlp->size > 4) {
        return pcietlp_set_error("cfgrd: size %d > 4", stlp->size);
    }

    encode_cfg_hdr(stlp, PCIE_TLP_TYPE_CFGRD0, rtlp);
    return tlpsz;
}

static int
decode_cfgrd(pcie_stlp_t *stlp, const void *rtlp, const size_t rtlpsz)
{
    const int tlpsz = 12;

    if (rtlpsz < tlpsz) {
        return pcietlp_set_error("cfgrd: rtlpsz want %d got %ld",
                                 tlpsz, rtlpsz);
    }

    decode_cfg_hdr(stlp, rtlp);
    return tlpsz;
}

/******************************************************************/

static int
encode_cfgwr(const pcie_stlp_t *stlp, void *rtlp, const size_t rtlpsz)
{
    const int tlpsz = 16;

    if (rtlpsz < tlpsz) {
        return pcietlp_set_error("cfgwr: rtlpsz want %d got %ld",
                                 tlpsz, rtlpsz);
    }

    encode_cfg_hdr(stlp, PCIE_TLP_TYPE_CFGWR0, rtlp);
    encode_data32(stlp, rtlp + 12);
    return tlpsz;
}

static int
decode_cfgwr(pcie_stlp_t *stlp, const void *rtlp, const size_t rtlpsz)
{
    const int tlpsz = 16;

    if (rtlpsz < tlpsz) {
        return pcietlp_set_error("cfgwr: rtlpsz want %d got %ld",
                                 tlpsz, rtlpsz);
    }

    decode_cfg_hdr(stlp, rtlp);
    decode_data32(stlp, rtlp + 12);
    return tlpsz;
}

/******************************************************************
 * MEM
 */

static int
encode_memrd(const pcie_stlp_t *stlp, void *rtlp, const size_t rtlpsz)
{
    const int tlpsz = 12;

    if (rtlpsz < tlpsz) {
        return pcietlp_set_error("memrd: rtlpsz want %d got %ld",
                                 tlpsz, rtlpsz);
    }

    encode_mem32_hdr(stlp, PCIE_TLP_TYPE_MEMRD, rtlp);
    encode_data(stlp, rtlp + 12);
    return tlpsz;
}

static int
decode_memrd(pcie_stlp_t *stlp, const void *rtlp, const size_t rtlpsz)
{
    const int tlpsz = 12;

    if (rtlpsz < tlpsz) {
        return pcietlp_set_error("memrd: rtlpsz want %d got %ld",
                                 tlpsz, rtlpsz);
    }

    decode_mem32_hdr(stlp, rtlp);
    return tlpsz;
}

/******************************************************************/

static int
encode_memwr(const pcie_stlp_t *stlp, void *rtlp, const size_t rtlpsz)
{
    const int tlpsz = 12 + stlp_dw(stlp) * 4;

    if (rtlpsz < tlpsz) {
        return pcietlp_set_error("memwr: rtlpsz want %d got %ld",
                                 tlpsz, rtlpsz);
    }
    if (stlp->size > 8) {
        /* stlp data is only 8 bytes */
        return pcietlp_set_error("memwr: size %d > 8", stlp->size);
    }

    encode_mem32_hdr(stlp, PCIE_TLP_TYPE_MEMWR, rtlp);
    encode_data(stlp, rtlp + 12);
    return tlpsz;
}

static int
decode_memwr(pcie_stlp_t *stlp, const void *rtlp, const size_t rtlpsz)
{
    const int tlpsz = 12;

    if (rtlpsz < tlpsz) {
        return pcietlp_set_error("memwr: rtlpsz want %d got %ld",
                                 tlpsz, rtlpsz);
    }

    decode_mem32_hdr(stlp, rtlp);

    if (rtlpsz < tlpsz + stlp->size) {
        return pcietlp_set_error("memwr: rtlpsz want %d got %ld",
                                 tlpsz + stlp->size, rtlpsz);
    }

    decode_data(stlp, rtlp + 12);
    return tlpsz + stlp->size;
}

/******************************************************************
 * MEM 64
 */

static int
encode_memrd64(const pcie_stlp_t *stlp, void *rtlp, const size_t rtlpsz)
{
    const int tlpsz = 16;

    if (rtlpsz < tlpsz) {
        return pcietlp_set_error("memrd64: rtlpsz want %d got %ld",
                                 tlpsz, rtlpsz);
    }

    encode_mem64_hdr(stlp, PCIE_TLP_TYPE_MEMRD64, rtlp);
    return tlpsz;
}

static int
decode_memrd64(pcie_stlp_t *stlp, const void *rtlp, const size_t rtlpsz)
{
    const int tlpsz = 16;

    if (rtlpsz < tlpsz) {
        return pcietlp_set_error("memrd64: rtlpsz want %d got %ld",
                                 tlpsz, rtlpsz);
    }

    decode_mem64_hdr(stlp, rtlp);
    return tlpsz;
}

/******************************************************************/

static int
encode_memwr64(const pcie_stlp_t *stlp, void *rtlp, const size_t rtlpsz)
{
    const int tlpsz = 16 + stlp_dw(stlp) * 4;

    if (rtlpsz < tlpsz) {
        return pcietlp_set_error("memwr64: rtlpsz want %d got %ld",
                                 tlpsz, rtlpsz);
    }
    if (stlp->size > 8) {
        /* stlp data is only 8 bytes */
        return pcietlp_set_error("memwr64: size %d > 8", stlp->size);
    }

    encode_mem64_hdr(stlp, PCIE_TLP_TYPE_MEMWR64, rtlp);
    encode_data(stlp, rtlp + 16);
    return tlpsz;
}

static int
decode_memwr64(pcie_stlp_t *stlp, const void *rtlp, const size_t rtlpsz)
{
    const int tlpsz = 16;

    if (rtlpsz < tlpsz) {
        return pcietlp_set_error("memwr64: rtlpsz want %d got %ld",
                                 tlpsz, rtlpsz);
    }

    decode_mem64_hdr(stlp, rtlp);

    if (rtlpsz < tlpsz + stlp->size) {
        return pcietlp_set_error("memwr64: rtlpsz want %d got %ld",
                                 tlpsz + stlp->size, rtlpsz);
    }

    decode_data(stlp, rtlp + 16);
    return tlpsz + stlp->size;
}

/******************************************************************
 * IO
 */

static int
encode_iord(const pcie_stlp_t *stlp, void *rtlp, const size_t rtlpsz)
{
    const int tlpsz = 12;

    if (rtlpsz < tlpsz) {
        return pcietlp_set_error("iord: rtlpsz want %d got %ld",
                                 tlpsz, rtlpsz);
    }
    if (stlp->size > 4) {
        return pcietlp_set_error("iord: size %d > 4", stlp->size);
    }

    encode_mem32_hdr(stlp, PCIE_TLP_TYPE_IORD, rtlp);
    return tlpsz;
}

static int
decode_iord(pcie_stlp_t *stlp, const void *rtlp, const size_t rtlpsz)
{
    const int tlpsz = 12;

    if (rtlpsz < tlpsz) {
        return pcietlp_set_error("iord: rtlpsz want %d got %ld",
                                 tlpsz, rtlpsz);
    }

    decode_mem32_hdr(stlp, rtlp);
    return tlpsz;
}

/******************************************************************/

static int
encode_iowr(const pcie_stlp_t *stlp, void *rtlp, const size_t rtlpsz)
{
    const int tlpsz = 16;

    if (rtlpsz < tlpsz) {
        return pcietlp_set_error("iowr: rtlpsz want %d got %ld",
                                 tlpsz, rtlpsz);
    }
    if (stlp->size > 4) {
        return pcietlp_set_error("iowr: size %d > 4", stlp->size);
    }

    encode_mem32_hdr(stlp, PCIE_TLP_TYPE_IOWR, rtlp);
    encode_data(stlp, rtlp + 12);
    return tlpsz;
}

static int
decode_iowr(pcie_stlp_t *stlp, const void *rtlp, const size_t rtlpsz)
{
    const int tlpsz = 16;

    if (rtlpsz < tlpsz) {
        return pcietlp_set_error("iowr: rtlpsz want %d got %ld",
                                 tlpsz, rtlpsz);
    }

    decode_mem32_hdr(stlp, rtlp);
    decode_data(stlp, rtlp + 12);
    return tlpsz;
}

/******************************************************************/

int
pcietlp_encode(const pcie_stlp_t *stlp, void *rtlp, const size_t rtlpsz)
{
    int n;

    pciesvc_memset(rtlp, 0, rtlpsz);
    pcietlp_clr_error();
    switch (stlp->type) {
    case PCIE_STLP_CFGRD:
        n = encode_cfgrd(stlp, rtlp, rtlpsz);
        break;
    case PCIE_STLP_CFGWR:
        n = encode_cfgwr(stlp, rtlp, rtlpsz);
        break;
    case PCIE_STLP_MEMRD:
        n = encode_memrd(stlp, rtlp, rtlpsz);
        break;
    case PCIE_STLP_MEMWR:
        n = encode_memwr(stlp, rtlp, rtlpsz);
        break;
    case PCIE_STLP_MEMRD64:
        n = encode_memrd64(stlp, rtlp, rtlpsz);
        break;
    case PCIE_STLP_MEMWR64:
        n = encode_memwr64(stlp, rtlp, rtlpsz);
        break;
    case PCIE_STLP_IORD:
        n = encode_iord(stlp, rtlp, rtlpsz);
        break;
    case PCIE_STLP_IOWR:
        n = encode_iowr(stlp, rtlp, rtlpsz);
        break;
    default:
        pcietlp_set_error("encode: unhandled type 0x%x", stlp->type);
        n = -1;
        break;
    }
    return pcietlp_is_error() ? -1 : n;
}

int
pcietlp_decode(pcie_stlp_t *stlp, const void *rtlp, const size_t rtlpsz)
{
    const pcie_tlp_common_hdr_t *hdr = rtlp;
    int n;

    pcietlp_clr_error();
    switch (hdr->type) {
    case PCIE_TLP_TYPE_MEMRD:
        stlp->type = PCIE_STLP_MEMRD;
        n = decode_memrd(stlp, rtlp, rtlpsz);
        break;
    case PCIE_TLP_TYPE_MEMRD64:
        stlp->type = PCIE_STLP_MEMRD64;
        n = decode_memrd64(stlp, rtlp, rtlpsz);
        break;
    case PCIE_TLP_TYPE_MEMWR:
        stlp->type = PCIE_STLP_MEMWR;
        n = decode_memwr(stlp, rtlp, rtlpsz);
        break;
    case PCIE_TLP_TYPE_MEMWR64:
        stlp->type = PCIE_STLP_MEMWR64;
        n = decode_memwr64(stlp, rtlp, rtlpsz);
        break;
    case PCIE_TLP_TYPE_IORD:
        stlp->type = PCIE_STLP_IORD;
        n = decode_iord(stlp, rtlp, rtlpsz);
        break;
    case PCIE_TLP_TYPE_IOWR:
        stlp->type = PCIE_STLP_IOWR;
        n = decode_iowr(stlp, rtlp, rtlpsz);
        break;
    case PCIE_TLP_TYPE_CFGRD0:
        stlp->type = PCIE_STLP_CFGRD;
        n = decode_cfgrd(stlp, rtlp, rtlpsz);
        break;
    case PCIE_TLP_TYPE_CFGWR0:
        stlp->type = PCIE_STLP_CFGWR;
        n = decode_cfgwr(stlp, rtlp, rtlpsz);
        break;
    case PCIE_TLP_TYPE_CFGRD1:
        stlp->type = PCIE_STLP_CFGRD1;
        n = decode_cfgrd(stlp, rtlp, rtlpsz);
        break;
    case PCIE_TLP_TYPE_CFGWR1:
        stlp->type = PCIE_STLP_CFGWR1;
        n = decode_cfgwr(stlp, rtlp, rtlpsz);
        break;
    default:
        pcietlp_set_error("decode: unhandled type 0x%x\n", hdr->type);
        n = -1;
        break;
    }
    return pcietlp_is_error() ? -1 : n;
}

/******************************************************************/

char *
pcietlp_buf(const pcie_stlp_t *stlp, void *buf, const size_t bufsz)
{
    switch (stlp->type) {
    case PCIE_STLP_CFGRD:
        pciesvc_snprintf(buf, bufsz,
                 "CFGRD %s 0x%08"PRIx64" size %d tag %d",
                 bdf_to_str(stlp->bdf), stlp->addr, stlp->size, stlp->tag);
        break;
    case PCIE_STLP_CFGWR:
        pciesvc_snprintf(buf, bufsz,
                 "CFGWR %s 0x%08"PRIx64" size %d tag %d 0x%0*"PRIx64"",
                 bdf_to_str(stlp->bdf), stlp->addr, stlp->size, stlp->tag,
                 stlp->size * 2, stlp->data);
        break;
    case PCIE_STLP_CFGRD1:
        pciesvc_snprintf(buf, bufsz,
                 "CFGRD1 %s 0x%08"PRIx64" size %d tag %d",
                 bdf_to_str(stlp->bdf), stlp->addr, stlp->size, stlp->tag);
        break;
    case PCIE_STLP_CFGWR1:
        pciesvc_snprintf(buf, bufsz,
                 "CFGWR1 %s 0x%08"PRIx64" size %d tag %d 0x%0*"PRIx64"",
                 bdf_to_str(stlp->bdf), stlp->addr, stlp->size, stlp->tag,
                 stlp->size * 2, stlp->data);
        break;
    case PCIE_STLP_MEMRD:
        pciesvc_snprintf(buf, bufsz,
                 "MEMRD 0x%08"PRIx64" size %d tag %d",
                 stlp->addr, stlp->size, stlp->tag);
        break;
    case PCIE_STLP_MEMWR:
        pciesvc_snprintf(buf, bufsz,
                 "MEMWR 0x%08"PRIx64" size %d tag %d 0x%0*"PRIx64"",
                 stlp->addr, stlp->size, stlp->tag,
                 stlp->size * 2, stlp->data);
        break;
    case PCIE_STLP_MEMRD64:
        pciesvc_snprintf(buf, bufsz,
                 "MEMRD64 0x%08"PRIx64" size %d tag %d",
                 stlp->addr, stlp->size, stlp->tag);
        break;
    case PCIE_STLP_MEMWR64:
        pciesvc_snprintf(buf, bufsz,
                 "MEMWR64 0x%08"PRIx64" size %d tag %d 0x%0*"PRIx64"",
                 stlp->addr, stlp->size, stlp->tag,
                 stlp->size * 2, stlp->data);
        break;
    case PCIE_STLP_IORD:
        pciesvc_snprintf(buf, bufsz,
                 "IORD 0x%08"PRIx64" size %d tag %d",
                 stlp->addr, stlp->size, stlp->tag);
        break;
    case PCIE_STLP_IOWR:
        pciesvc_snprintf(buf, bufsz,
                 "IOWR 0x%08"PRIx64" size %d tag %d 0x%0*"PRIx64"",
                 stlp->addr, stlp->size, stlp->tag,
                 stlp->size * 2, stlp->data);
        break;
    case PCIE_STLP_MALFORMED:
        pciesvc_snprintf(buf, bufsz, "MALFORMED");
        break;
    default:
        pciesvc_snprintf(buf, bufsz, "UNKNOWN type %d", stlp->type);
        break;
    }
    return buf;
}

char *
pcietlp_str(const pcie_stlp_t *stlp)
{
    static char buf[80];

    return pcietlp_buf(stlp, buf, sizeof(buf));
}
