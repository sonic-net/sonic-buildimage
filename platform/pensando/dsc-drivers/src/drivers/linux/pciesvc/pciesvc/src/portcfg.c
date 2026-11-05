// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2018-2021, Pensando Systems Inc.
 */

#include "pciesvc_impl.h"
#include "portcfg.h"

typedef union {
    u_int32_t d;
    u_int16_t w[2];
    u_int8_t  b[4];
} cfgdata_t;

static u_int64_t
portcfg_base(const int port)
{
    return PXC_(DHS_C_MAC_APB_ENTRY, port);
}

u_int32_t
portcfg_readdw(const int port, const u_int16_t addr)
{
    u_int32_t v;
    pciesvc_assert(addr < 4096);
    pciesvc_pciepreg_rd32(portcfg_base(port) + addr, &v);
    return v;
}

void
portcfg_writedw(const int port, const u_int16_t addr, u_int32_t val)
{
    pciesvc_assert(addr < 4096);
    pciesvc_pciepreg_wr32(portcfg_base(port) + addr, val);
}

u_int8_t
portcfg_readb(const int port, const u_int16_t addr)
{
    const u_int16_t addrdw = addr & ~0x3;
    const u_int8_t byteidx = addr & 0x3;
    cfgdata_t v;

    v.d = portcfg_readdw(port, addrdw);
    return v.b[byteidx];
}

u_int16_t
portcfg_readw(const int port, const u_int16_t addr)
{
    const u_int16_t addrdw = addr & ~0x3;
    const u_int8_t wordidx = (addr & 0x3) >> 1;
    cfgdata_t v;

    pciesvc_assert((addr & 0x1) == 0);
    v.d = portcfg_readdw(port, addrdw);
    return v.w[wordidx];
}

u_int32_t
portcfg_readd(const int port, const u_int16_t addr)
{
    pciesvc_assert((addr & 0x3) == 0);
    return portcfg_readdw(port, addr);
}

void
portcfg_writeb(const int port, const u_int16_t addr, const u_int8_t val)
{
    const u_int16_t addrdw = addr & ~0x3;
    const u_int8_t byteidx = addr & 0x3;
    cfgdata_t v;

    v.d = portcfg_readdw(port, addrdw);
    v.b[byteidx] = val;
    portcfg_writedw(port, addrdw, v.d);
}

void
portcfg_writew(const int port, const u_int16_t addr, const u_int16_t val)
{
    const u_int16_t addrdw = addr & ~0x3;
    const u_int8_t wordidx = (addr & 0x3) >> 1;
    cfgdata_t v;

    pciesvc_assert((addr & 0x1) == 0);
    v.d = portcfg_readdw(port, addrdw);
    v.w[wordidx] = val;
    portcfg_writedw(port, addrdw, v.d);
}

void
portcfg_writed(const int port, const u_int16_t addr, const u_int32_t val)
{
    pciesvc_assert((addr & 0x3) == 0);
    portcfg_writedw(port, addr, val);
}

void
portcfg_read_bus(const int port,
                 u_int8_t *pribus, u_int8_t *secbus, u_int8_t *subbus)
{
    cfgdata_t v;

    v.d = portcfg_readdw(port, PCI_PRIMARY_BUS);

    if (pribus) *pribus = v.b[0];
    if (secbus) *secbus = v.b[1];
    if (subbus) *subbus = v.b[2];
}
