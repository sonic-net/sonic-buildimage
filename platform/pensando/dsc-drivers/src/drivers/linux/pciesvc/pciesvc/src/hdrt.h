/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2017,2021, Pensando Systems Inc.
 */

#ifndef __HDRT_H__
#define __HDRT_H__

typedef struct {
    u_int64_t valid     :1;
    u_int64_t bdf       :16;
    u_int64_t td        :1;
    u_int64_t pasid_en  :1;
    u_int64_t pasid_sel :2;
    u_int64_t pasid     :20;
    u_int64_t pasid_exe :1;
    u_int64_t pasid_priv:1;
    u_int64_t attr2_1_rd:2;
    u_int64_t attr2_1_wr:2;
    u_int64_t rc_cfg1   :1;
    u_int64_t attr0_rd  :1;
    u_int64_t attr0_wr  :1;
    u_int64_t ats_at_wr :1;
    u_int64_t ats_at_rd :1;
    u_int64_t tc        :3;
    u_int64_t ln_wr     :1;
    u_int64_t ln_rd     :1;
    u_int64_t rsrv      :13;
    u_int64_t ecc       :8;
    u_int64_t _pad      :16;
} __attribute__((packed)) hdrt_t;

void pciehw_hdrt_init(void);
int pciehw_hdrt_load(const u_int32_t lifb,
                     const u_int32_t lifc,
                     const u_int16_t bdf);
int pciehw_hdrt_unload(const u_int32_t lifb,
                       const u_int32_t lifc);

#endif /* __HDRT_H__ */
