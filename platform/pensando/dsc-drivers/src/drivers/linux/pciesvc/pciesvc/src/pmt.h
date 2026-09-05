/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2017-2018,2021, Pensando Systems Inc.
 */

#ifndef __PMT_H__
#define __PMT_H__

struct pmt_s;
typedef struct pmt_s pmt_t;

void pmt_get(const int pmti, pmt_t *pmt);
void pmt_set(const int pmti, const pmt_t *pmt);

void pmt_bar_setaddr(pmt_t *pmt, const u_int64_t addr);
void pmt_bar_setaddrm(pmt_t *pmt, const u_int64_t addr, const u_int64_t mask);
u_int64_t pmt_bar_getaddr(const pmt_t *pmt);

void pmt_bar_set_bdf(pmt_t *pmt, const u_int16_t bdf);
void pmt_cfg_set_bus(pmt_t *pmt, const u_int8_t bus);

union pciehwbar_s;
typedef union pciehwbar_u pciehwbar_t;

void pciehw_pmt_setaddr(pciehwbar_t *phwbar, u_int64_t addr);
void pciehw_bar_load_pmts(pciehwbar_t *phwbar);
void pciehw_bar_unload_pmts(pciehwbar_t *phwbar);
void pciehw_bar_load_ovrds(pciehwbar_t *phwbar);
void pciehw_bar_unload_ovrds(pciehwbar_t *phwbar);
void pciehw_pmt_load_cfg(pciehwdev_t *phwdev);
void pciehw_pmt_unload_cfg(pciehwdev_t *phwdev);
int pciehw_pmt_adjust_vf0(pciehw_spmt_t *spmt,
                          const u_int64_t addr,
                          const int numvfs,
                          const int do_log);

#endif /* __PMT_H__ */
