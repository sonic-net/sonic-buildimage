/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021-2022, Pensando Systems Inc.
 */

#ifndef __PCIESVC_LOCAL_H__
#define __PCIESVC_LOCAL_H__

#ifdef __cplusplus
extern "C" {
#if 0
} /* close to calm emacs autoindent */
#endif
#endif

union pciehwdev_u; typedef union pciehwdev_u pciehwdev_t;
union pciehwbar_u; typedef union pciehwbar_u pciehwbar_t;
typedef u_int32_t pciehwdevh_t;

u_int64_t pciehw_bar_getsize(pciehwbar_t *phwbar);
void pciehw_bar_setaddr(pciehwbar_t *phwbar, const u_int64_t addr);
void pciehw_bar_load_ovrds(pciehwbar_t *phwbar);
void pciehw_bar_unload_ovrds(pciehwbar_t *phwbar);
void pciehw_bar_load(pciehwdev_t *phwdev, pciehwbar_t *phwbar);
void pciehw_cfg_load(pciehwdev_t *phwdev);
void pciehw_pmt_setaddr(pciehwbar_t *phwbar, const u_int64_t addr);
void pciehw_reset_bus(pciehwdev_t *phwdev, const u_int8_t bus);
uint32_t pciehw_vpd_read(pciehwdevh_t hwdevh, const uint16_t addr);
void pciehw_vpd_write(pciehwdevh_t hwdevh,
                      const uint16_t addr, const uint32_t data);

u_int16_t pciehwdev_get_hostbdf(const pciehwdev_t *phwdev);

void pciehw_sriov_ctrl(pciehwdev_t *phwdev,
                       const u_int16_t ctrl, const u_int16_t numvfs);

struct pmt_s; typedef struct pmt_s pmt_t;
int pmt_reserve_vf0adj(const int n);
int pmt_alloc(const int n, const int pri);
void pmt_free(const int pmtb, const int pmtc);
void pmt_get(const int pmti, pmt_t *pmt);
void pmt_set(const int pmti, const pmt_t *pmt);
void pmt_bar_set_bdf(pmt_t *pmt, const u_int16_t bdf);
u_int64_t pmt_bar_getaddr(const pmt_t *pmt);
void pmt_bar_setaddr(pmt_t *pmt, const u_int64_t addr);

union pmt_entry_u; typedef union pmt_entry_u pmt_entry_t;
struct pmt_datamask_s; typedef struct pmt_datamask_s pmt_datamask_t;
void pmt_entry_enc(pmt_entry_t *pmte, const pmt_datamask_t *dm);
void pmt_entry_dec(const pmt_entry_t *pmte, pmt_datamask_t *dm);

union prt_u; typedef union prt_u prt_t;
int prt_alloc(const int n);
void prt_free(const int prtbase, const int prtcount);
void prt_get(const int prti, prt_t *prt);
void prt_set(const int prti, const prt_t *prt);

#ifdef __cplusplus
}
#endif

#endif /* __PCIESVC_LOCAL_H__ */
