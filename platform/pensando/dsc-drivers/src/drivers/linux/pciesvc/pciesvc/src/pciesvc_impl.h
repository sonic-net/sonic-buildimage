/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021-2022, Pensando Systems Inc.
 */

#ifndef __PCIESVC_IMPL_H__
#define __PCIESVC_IMPL_H__

#ifdef __cplusplus
extern "C" {
#if 0
} /* close to calm emacs autoindent */
#endif
#endif

#include "pciesvc_system.h"
#include "pciesvc.h"
#include "notify_entry.h"
#include "cfgspace.h"
#include "portcfg.h"
#include "log.h"
#include "asic_regs.h"
#include "pmt.h"

struct indirect_entry_s; typedef struct indirect_entry_s indirect_entry_t;
struct notify_entry_s; typedef struct notify_entry_s notify_entry_t;

void pciehw_cfgrd_indirect(const int port, indirect_entry_t *ientry);
void pciehw_cfgwr_indirect(const int port, indirect_entry_t *ientry);
void pciehw_barrd_indirect(const int port, indirect_entry_t *ientry);
void pciehw_barwr_indirect(const int port, indirect_entry_t *ientry);

void pciehw_cfgrd_notify(const int port, notify_entry_t *nentry);
void pciehw_cfgwr_notify(const int port, notify_entry_t *nentry);
void pciehw_barrd_notify(const int port, notify_entry_t *nentry);
void pciehw_barwr_notify(const int port, notify_entry_t *nentry);

enum pciesvc_rsttype_e; typedef enum pciesvc_rsttype_e pciesvc_rsttype_t;
void pciehw_cfg_reset(pciehwdev_t *phwdev, const pciesvc_rsttype_t rsttype);

u_int64_t pciehw_bar_getsize(pciehwbar_t *phwbar);
void pciehw_bar_setaddr(pciehwbar_t *phwbar, const u_int64_t addr);
void pciehw_bar_load(pciehwdev_t *phwdev, pciehwbar_t *phwbar);
void pciehw_bar_enable(pciehwdev_t *phwdev, pciehwbar_t *phwbar, const int on);

u_int16_t pciehwdev_get_hostbdf(const pciehwdev_t *phwdev);

#define CLEAN                   0
#define DIRTY                   1

static inline void
pciesvc_reg_rd32w(const uint64_t pa, uint32_t *w, const uint32_t nw)
{
    int i;

    for (i = 0; i < nw; i++) {
        w[i] = pciesvc_reg_rd32(pa + (i * 4));
    }
}

static inline void
pciesvc_reg_wr32w(const uint64_t pa, const uint32_t *w, const uint32_t nw)
{
    int i;

    for (i = 0; i < nw; i++) {
        pciesvc_reg_wr32(pa + (i * 4), w[i]);
    }
}

static inline uint64_t
pciesvc_reg_rd64(const uint64_t pa)
{
    uint64_t val;
    uint32_t *w = (uint32_t *)&val;

    pciesvc_reg_rd32w(pa, w, 2);
    return val;
}

static inline void
pciesvc_reg_wr64(const uint64_t pa, const uint64_t val)
{
    const uint32_t *w = (const uint32_t *)&val;

    pciesvc_reg_wr32w(pa, w, 2);
}

static inline uint64_t
pciesvc_indirect_intr_dest_pa(const int port)
{
    static uint64_t intr_dest_pa[PCIEHW_NPORTS];

    pciesvc_assert(port >= 0 && port < PCIEHW_NPORTS);
    if (intr_dest_pa[port] == 0) {
        pciehw_mem_t *phwmem = pciesvc_hwmem_get();
        pciehw_shmem_t *pshmem = pciesvc_shmem_get();
        intr_dest_pa[port] =
            pciesvc_vtop(PHWMEM_ADDR_FIELD(phwmem, pshmem, indirect_intr_dest[port]));
    }
    return intr_dest_pa[port];
}

static inline uint64_t
pciesvc_notify_intr_dest_pa(const int port)
{
    static uint64_t intr_dest_pa[PCIEHW_NPORTS];

    pciesvc_assert(port >= 0 && port < PCIEHW_NPORTS);
    if (intr_dest_pa[port] == 0) {
        pciehw_mem_t *phwmem = pciesvc_hwmem_get();
        pciehw_shmem_t *pshmem = pciesvc_shmem_get();
        intr_dest_pa[port] =
            pciesvc_vtop(PHWMEM_ADDR_FIELD(phwmem, pshmem, notify_intr_dest[port]));
    }
    return intr_dest_pa[port];
}

static inline uint64_t
pciesvc_cfgcur_pa(void)
{
    static uint64_t cfgcur_pa;

    if (cfgcur_pa == 0) {
        pciehw_mem_t *phwmem = pciesvc_hwmem_get();
        pciehw_shmem_t *pshmem = pciesvc_shmem_get();
        cfgcur_pa = pciesvc_vtop(PHWMEM_DATA_FIELD(phwmem, pshmem, cfgcur));
    }
    return cfgcur_pa;
}

static inline uint64_t
pciesvc_notify_ring_mask(const int port)
{
    static uint64_t ring_mask;

    if (ring_mask == 0) {
        pciehw_shmem_t *pshmem = pciesvc_shmem_get();
        ring_mask = PSHMEM_DATA_FIELD(pshmem, notify_ring_mask);
    }
    return ring_mask;
}

static inline notify_entry_t *
pciesvc_notify_ring_get(const int port, const int idx)
{
    pciehw_mem_t *phwmem = pciesvc_hwmem_get();
    pciehw_shmem_t *pshmem = pciesvc_shmem_get();
    notify_entry_t *notify_ring;

    notify_ring = (notify_entry_t *)PHWMEM_ADDR_FIELD(phwmem, pshmem, notify_area[port]);
    return &notify_ring[idx];
}

static inline void
pciesvc_notify_ring_put(const notify_entry_t *nentry)
{
    /* nop */
}

static inline pciehw_port_t *
pciesvc_port_get(const int port)
{
    pciehw_shmem_t *pshmem = pciesvc_shmem_get();

    pciesvc_assert(port >= 0 && port <= PCIEHW_NPORTS);
    return PSHMEM_ADDR_FIELD(pshmem, port[port]);
}

static inline void
pciesvc_port_put(const pciehw_port_t *p, const int dirty)
{
    /* nop */
}

static inline pciehwdev_t *
pciesvc_dev_get(const pciehwdevh_t hwdevh)
{
    pciehw_shmem_t *pshmem = pciesvc_shmem_get();
    int ndevs = PSHMEM_NDEVS(pshmem);

    return hwdevh > 0 &&
        hwdevh < ndevs ? PSHMEM_ADDR_FIELD(pshmem, dev[hwdevh]) : NULL;
}

static inline void
pciesvc_dev_put(const pciehwdev_t *phwdev, const int dirty)
{
    /* nop */
}

static inline void
pciesvc_cfgspace_get(const pciehwdevh_t hwdevh, cfgspace_t *cs)
{
    pciehw_mem_t *phwmem = pciesvc_hwmem_get();
    pciehw_shmem_t *pshmem = pciesvc_shmem_get();

    cs->cur = PHWMEM_DATA_FIELD(phwmem, pshmem, cfgcur[hwdevh]);
    cs->msk = PSHMEM_DATA_FIELD(pshmem, cfgmsk[hwdevh]);
    cs->rst = PSHMEM_DATA_FIELD(pshmem, cfgrst[hwdevh]);
    cs->size = PCIEHW_CFGSZ;
}

static inline void
pciesvc_cfgspace_put(const cfgspace_t *cs, const int dirty)
{
    /* nop */
}

static inline pciehw_spmt_t *
pciesvc_spmt_get(const int idx)
{
    pciehw_shmem_t *pshmem = pciesvc_shmem_get();

    return PSHMEM_ADDR_FIELD(pshmem, spmt[idx]);
}

static inline void
pciesvc_spmt_put(const pciehw_spmt_t *spmt, const int dirty)
{
    /* nop */
}

static inline pciehw_sprt_t *
pciesvc_sprt_get(const int idx)
{
    pciehw_shmem_t *pshmem = pciesvc_shmem_get();

    return PSHMEM_ADDR_FIELD(pshmem, sprt[idx]);
}

static inline void
pciesvc_sprt_put(const pciehw_sprt_t *sprt, const int dirty)
{
    /* nop */
}

static inline void *
pciesvc_vpd_get(const pciehwdevh_t hwdevh)
{
    pciehw_shmem_t *pshmem = pciesvc_shmem_get();

    return PSHMEM_ADDR_FIELD(pshmem, vpddata[hwdevh]);
}

static inline void
pciesvc_vpd_put(const void *vpddata, const int dirty)
{
    /* nop */
}

static inline pciehwdev_t *
pciehwdev_get(const pciehwdevh_t hwdevh)
{
    pciehwdev_t *phwdev = pciesvc_dev_get(hwdevh);

    /* older fw versions didn't init hwdevh, set it now */
    if (phwdev && phwdev->hwdevh != hwdevh) {
        phwdev->hwdevh = hwdevh;
    }
    return phwdev;
}

static inline void
pciehwdev_put(const pciehwdev_t *phwdev, const int dirty)
{
    pciesvc_dev_put(phwdev, dirty);
}

static inline pciehwdevh_t
pciehwdev_geth(const pciehwdev_t *phwdev)
{
    return phwdev ? phwdev->hwdevh : 0;
}

static inline const char *
pciehwdev_get_name(const pciehwdev_t *phwdev)
{
    return phwdev->name;
}

static inline pciehwdev_t *
pciehwdev_vfdev_get(const pciehwdev_t *phwdev, const int vfidx)
{
    pciesvc_assert(vfidx >= 0 && vfidx < phwdev->totalvfs);
    return pciehwdev_get(phwdev->childh + vfidx);
}

static inline void
pciehwdev_vfdev_put(const pciehwdev_t *phwdev, const int dirty)
{
    pciehwdev_put(phwdev, dirty);
}

/*
 * roundup_power2 - Round up to next power of 2.
 */
static inline u_int64_t
roundup_power2(u_int64_t n)
{
    while (n & (n - 1)) {
        n = (n | (n - 1)) + 1;
    }
    return n;
}

static inline u_int64_t
rounddn_power2(u_int64_t n)
{
    return roundup_power2(n + 1) >> 1;
}

static inline u_int64_t
align_to(u_int64_t n, u_int64_t align)
{
	return (n + align - 1) & ~(align - 1);
}

#ifdef __cplusplus
}
#endif

#endif /* __PCIESVC_IMPL_H__ */
