/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2018,2021, Pensando Systems Inc.
 */

#ifndef __PORTCFG_H__
#define __PORTCFG_H__

#ifdef __cplusplus
extern "C" {
#if 0
} /* close to calm emacs autoindent */
#endif
#endif

#define PORTCFG_CAP_PCIE        0x80
#define PORTCFG_CAP_AER         0x200
#define PORTCFG_CAP_PHYSLAYER   0x340   /* Gen4 Physical Layer */

/* rename these to avoid static link dups */
#define portcfg_readb           _pciesvc_portcfg_readb
#define portcfg_readw           _pciesvc_portcfg_readw
#define portcfg_readd           _pciesvc_portcfg_readd
#define portcfg_writeb          _pciesvc_portcfg_writeb
#define portcfg_writew          _pciesvc_portcfg_writew
#define portcfg_writed          _pciesvc_portcfg_writed
#define portcfg_read_bus        _pciesvc_portcfg_read_bus

void portcfg_read_bus(const int port,
                      u_int8_t *pribus, u_int8_t *secbus, u_int8_t *subbus);

u_int8_t  portcfg_readb(const int port, const u_int16_t addr);
u_int16_t portcfg_readw(const int port, const u_int16_t addr);
u_int32_t portcfg_readd(const int port, const u_int16_t addr);

void portcfg_writeb(const int port, const u_int16_t addr, const u_int8_t val);
void portcfg_writew(const int port, const u_int16_t addr, const u_int16_t val);
void portcfg_writed(const int port, const u_int16_t addr, const u_int32_t val);

#ifdef __cplusplus
}
#endif

#endif /* __PORTCFG_H__ */
