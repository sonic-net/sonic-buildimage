/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2018,2020, Pensando Systems Inc.
 */

#ifndef __TLPAUXINFO_H__
#define __TLPAUXINFO_H__

/*
 * For indirect or notify transactions, the hardware delivers
 * this auxiliary information along with the pcie tlp.
 */
typedef struct tlpauxinfo_s {
#if defined(ASIC_CAPRI)
    uint64_t direct_endaddr     :6;
#elif defined(ASIC_ELBA)
    uint64_t spare              :3;
    uint64_t wqebpdbxen         :1;
    uint64_t wqebpsize          :2;
#else
#error "ASIC not specified"
#endif
    uint64_t direct_blen        :4;
    uint64_t is_indirect        :1;
    uint64_t is_direct          :1;
    uint64_t is_ur              :1;
    uint64_t is_ca              :1;
    uint64_t romsksel           :7;
    uint64_t context_id         :7;
    uint64_t vfid               :11;
    uint64_t is_notify          :1;
    uint64_t direct_size        :9;
    uint64_t direct_addr        :52;
    uint64_t aspace             :1;
    uint64_t pmti               :10;
    uint64_t pmt_hit            :1;
    uint64_t indirect_reason    :5;
    uint64_t is_host            :1;
    uint64_t axilen             :4;
#if defined(ASIC_CAPRI)
    uint64_t rsrv               :3;
#elif defined(ASIC_ELBA)
    uint64_t rsrv               :1;
    uint64_t wqetype            :1;     /* wqe type, 0=wqe, 1=doorbell */
    uint64_t wqebpdben          :1;     /* wqe bypass doorbell enable */
#else
#error "ASIC not specified"
#endif
    uint64_t eop                :1;
    uint64_t sop                :1;
} __attribute__((packed)) tlpauxinfo_t;

#endif /* __TLPAUXINFO_H__ */
