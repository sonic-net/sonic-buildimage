/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2018-2019,2022, Pensando Systems Inc.
 */

#ifndef __INTRUTILS_H__
#define __INTRUTILS_H__

#ifdef __cplusplus
extern "C" {
#if 0
} /* close to calm emacs autoindent */
#endif
#endif

typedef struct intr_drvcfg_s {
    u_int32_t coal_init;
    u_int32_t mask;
    u_int32_t int_credits;
    u_int32_t mask_on_assert;
    u_int32_t coal_curr;
} __attribute__((packed)) intr_drvcfg_t;

typedef struct intr_msixcfg_s {
    u_int64_t msgaddr;
    u_int32_t msgdata;
    u_int32_t vector_ctrl;
} __attribute__((packed)) intr_msixcfg_t;

typedef union intr_fwcfg_u {
    struct {
        u_int32_t function_mask:1;
        u_int32_t rsrv:31;
        u_int32_t lif:11;
        u_int32_t port_id:3;
        u_int32_t local_int:1;
        u_int32_t legacy:1;
        u_int32_t int_pin:2;
        u_int32_t rsrv2:14;
    } __attribute__((packed));
    u_int32_t w[2];
} intr_fwcfg_t;

typedef union intr_state_s {
    struct {
        u_int64_t msixcfg_msg_addr_51_2:50;
        u_int64_t msixcfg_msg_data:32;
        u_int64_t msixcfg_vector_ctrl:1;
        u_int64_t fwcfg_function_mask:1;
        u_int64_t fwcfg_lif:11;
        u_int64_t fwcfg_local_int:1;
        u_int64_t fwcfg_legacy_int:1;
        u_int64_t fwcfg_legacy_pin:2;
        u_int64_t drvcfg_mask:1;
          int64_t drvcfg_int_credits:16; /* signed */
        u_int64_t drvcfg_mask_on_assert:1;
        u_int64_t fwcfg_port_id:3;
    } __attribute__((packed));
    u_int32_t w[4];
} intr_state_t;

/* override these to avoid static link dups */
#define intr_assert             _pciesvc_intr_assert
#define intr_deassert           _pciesvc_intr_deassert
#define intr_drvcfg_mask        _pciesvc_intr_drvcfg_mask
#define intr_fwcfg_mode         _pciesvc_intr_fwcfg_mode
#define intr_reset_pci          _pciesvc_intr_reset_pci

void intr_assert(const int intr);
void intr_deassert(const int intr);
int intr_drvcfg_mask(const int intr, const int on);
void intr_fwcfg_mode(const int intr, const int legacy, const int fmask);

/*
 * intr_reset_pci() - reset the pcie managed register groups to default values,
 *                    use for pcie block resets (FLR, bus reset).
 */
void intr_reset_pci(const int intrb, const int intrc, const int dmask);

#ifdef __cplusplus
}
#endif

#endif /* __INTRUTILS_H__ */
