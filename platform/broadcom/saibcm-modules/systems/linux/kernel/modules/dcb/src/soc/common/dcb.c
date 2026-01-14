/*
 * $Id$
 *
 * $Copyright: 2017-2025 Broadcom Inc. All rights reserved.
 * 
 * Permission is granted to use, copy, modify and/or distribute this
 * software under either one of the licenses below.
 * 
 * License Option 1: GPL
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2, as
 * published by the Free Software Foundation (the "GPL").
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License version 2 (GPLv2) for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * version 2 (GPLv2) along with this source code.
 * 
 * 
 * License Option 2: Broadcom Open Network Switch APIs (OpenNSA) license
 * 
 * This software is governed by the Broadcom Open Network Switch APIs license:
 * https://www.broadcom.com/products/ethernet-connectivity/software/opennsa $
 * 
 * 
 *
 * File:        dcb.c
 * Purpose:     DCB manipulation routines
 *              Provide a uniform means of manipulation of DMA control blocks
 *              that is independent of the actual DCB format used in any
 *              particular chip.
 */

#include <shared/bsl.h>

#include <soc/types.h>
#include <soc/drv.h>
#include <soc/dcb.h>
#include <soc/dcbformats.h>
#include <soc/higig.h>
#include <soc/rx.h>
#include <shared/bsl.h>

#if defined(BCM_ESW_SUPPORT)

#define GETHDRFUNCEXPR(_dt, _name, _expr)                                        \
        static uint32 ep_to_cpu_hdr##_dt##_##_name##_get(void *hdr) {            \
                ep_to_cpu_hdr##_dt##_t *h = (ep_to_cpu_hdr##_dt##_t *)hdr;       \
                return _expr;                                                    \
        }
#define GETHDRFUNCFIELD(_dt, _name, _field)                                      \
        GETHDRFUNCEXPR(_dt, _name, h->_field)
#define GETHDRFUNCERR(_dt, _name)                                                \
        static uint32 ep_to_cpu_hdr##_dt##_##_name##_get(void *hdr) {            \
                COMPILER_REFERENCE(hdr);                                         \
                ep_to_cpu_hdr_funcerr(_dt, #_name "_get");                       \
                return 0;                                                        \
        }
#define GETHDRFUNCNULL(_dt, _name)                                               \
        static uint32 ep_to_cpu_hdr##_dt##_##_name##_get(void *hdr) {            \
                COMPILER_REFERENCE(hdr);                                         \
                return 0;                                                        \
        }
#define GETHDRPTREXPR(_dt, _name, _expr)                                         \
        static uint32 * ep_to_cpu_hdr##_dt##_##_name##_get(void *hdr) {          \
                ep_to_cpu_hdr##_dt##_t *h = (ep_to_cpu_hdr##_dt##_t *)hdr;       \
                return _expr;                                                    \
        }
#define GETHDRPTRERR(_dt, _name)                                                 \
        static uint32 * ep_to_cpu_hdr##_dt##_##_name##_get(void *hdr) {          \
                COMPILER_REFERENCE(hdr);                                         \
                ep_to_cpu_hdr_funcerr(_dt, #_name "_get");                       \
                return NULL;                                                     \
        }
#define GETHDRFUNCUNITEXPR(_dt, _name, _expr)                                    \
        static uint32 ep_to_cpu_hdr##_dt##_##_name##_get(int unit, void *hdr) {  \
                ep_to_cpu_hdr##_dt##_t *h = (ep_to_cpu_hdr##_dt##_t *)hdr;       \
                COMPILER_REFERENCE(unit);                                        \
                return _expr;                                                    \
        }
#define GETHDRFUNCUNITFIELD(_dt, _name, _field)                                  \
        GETHDRFUNCUNITEXPR(_dt, _name, h->_field)
#define GETHDRFUNCUNITERR(_dt, _name)                                            \
        static uint32 ep_to_cpu_hdr##_dt##_##_name##_get(int unit, void *hdr) {  \
                COMPILER_REFERENCE(unit);                                        \
                COMPILER_REFERENCE(hdr);                                         \
                ep_to_cpu_hdr_funcerr(_dt, #_name "_get");                       \
                return 0;                                                        \
        }
#define SETHDRFUNCEXPR(_dt, _name, _arg, _expr)                                  \
        static void ep_to_cpu_hdr##_dt##_##_name##_set(void *hdr, _arg) {        \
                ep_to_cpu_hdr##_dt##_t *h = (ep_to_cpu_hdr##_dt##_t *)hdr;       \
                _expr;                                                           \
        }
#define SETHDRFUNCFIELD(_dt, _name, _field, _arg, _expr)                         \
        SETHDRFUNCEXPR(_dt, _name, _arg, h->_field = _expr)
#define SETHDRFUNCERR(_dt, _name, _type)                                         \
        static void ep_to_cpu_hdr##_dt##_##_name##_set(void *hdr, _type val) {   \
                COMPILER_REFERENCE(hdr);                                         \
                COMPILER_REFERENCE(val);                                         \
                ep_to_cpu_hdr_funcerr(_dt, #_name "_set");                       \
        }
#define SETHDRFUNCNULL(_dt, _name)                                                \
        static uint32 ep_to_cpu_hdr##_dt##_##_name##_set(void *hdr, uint32 val) { \
                COMPILER_REFERENCE(hdr);                                          \
                return 0;                                                         \
        }
#define SETHDRFUNCEXPRIGNORE(_dt, _name, _arg, _expr)                               \
        SETHDRFUNCEXPR(_dt, _name, _arg, COMPILER_REFERENCE(h))
#if defined(LE_HOST)
#define GETHDRHGFUNCEXPR(_dt, _name, _expr)                                      \
        static uint32 ep_to_cpu_hdr##_dt##_##_name##_get(void *hdr) {            \
                ep_to_cpu_hdr##_dt##_t *hd = (ep_to_cpu_hdr##_dt##_t *)hdr;      \
                uint32  hgh[3];                                                  \
                soc_higig_hdr_t *h = (soc_higig_hdr_t *)&hgh[0];                 \
                hgh[0] = soc_htonl(hd->mh0);                                     \
                hgh[1] = soc_htonl(hd->mh1);                                     \
                hgh[2] = soc_htonl(hd->mh2);                                     \
                return _expr;                                                    \
        }
#else
#define GETHDRHGFUNCEXPR(_dt, _name, _expr)                                      \
        static uint32 ep_to_cpu_hdr##_dt##_##_name##_get(void *hdr) {            \
                ep_to_cpu_hdr##_dt##_t *hd = (ep_to_cpu_hdr##_dt##_t *)hdr;      \
                soc_higig_hdr_t *h = (soc_higig_hdr_t *)&hd->mh0;                \
                return _expr;                                                    \
        }
#endif
#define GETHDRHGFUNCFIELD(_dt, _name, _field)                                    \
        GETHDRHGFUNCEXPR(_dt, _name, h->hgp_overlay1._field)

#if defined(LE_HOST)
#define GETHDRHG2FUNCEXPR(_dt, _name, _expr)                                     \
        static uint32 ep_to_cpu_hdr##_dt##_##_name##_get(void *hdr) {            \
                ep_to_cpu_hdr##_dt##_t *hd = (ep_to_cpu_hdr##_dt##_t *)hdr;      \
                uint32  hgh[4];                                                  \
                soc_higig2_hdr_t *h = (soc_higig2_hdr_t *)&hgh[0];               \
                hgh[0] = soc_htonl(hd->mh0);                                     \
                hgh[1] = soc_htonl(hd->mh1);                                     \
                hgh[2] = soc_htonl(hd->mh2);                                     \
                hgh[3] = soc_htonl(hd->mh3);                                     \
                return _expr;                                                    \
        }
#else
#define GETHDRHG2FUNCEXPR(_dt, _name, _expr)                                     \
        static uint32 ep_to_cpu_hdr##_dt##_##_name##_get(void *hdr) {            \
                ep_to_cpu_hdr##_dt##_t *hd = (ep_to_cpu_hdr##_dt##_t *)hdr;      \
                soc_higig2_hdr_t *h = (soc_higig2_hdr_t *)&hd->mh0;              \
                return _expr;                                                    \
        }
#endif
#define GETHDRHG2FUNCFIELD(_dt, _name, _field)                                   \
        GETHDRHG2FUNCEXPR(_dt, _name, h->ppd_overlay1._field)

#if defined(LE_HOST)
#define GETHDRHGFUNCUNITEXPR(_dt, _name, _expr)                                  \
        static uint32 ep_to_cpu_hdr##_dt##_##_name##_get(int unit, void *hdr) {  \
                ep_to_cpu_hdr##_dt##_t *hd = (ep_to_cpu_hdr##_dt##_t *)hdr;      \
                uint32  hgh[3];                                                  \
                soc_higig_hdr_t *h = (soc_higig_hdr_t *)&hgh[0];                 \
                COMPILER_REFERENCE(unit);                                        \
                hgh[0] = soc_htonl(hd->mh0);                                     \
                hgh[1] = soc_htonl(hd->mh1);                                     \
                hgh[2] = soc_htonl(hd->mh2);                                     \
                return _expr;                                                    \
        }
#else
#define GETHDRHGFUNCUNITEXPR(_dt, _name, _expr)                                  \
        static uint32 ep_to_cpu_hdr##_dt##_##_name##_get(int unit, void *hdr) {  \
                ep_to_cpu_hdr##_dt##_t *hd = (ep_to_cpu_hdr##_dt##_t *)dcb;      \
                soc_higig_hdr_t *h = (soc_higig_hdr_t *)&hd->mh0;                \
                COMPILER_REFERENCE(unit);                                        \
                return _expr;                                                    \
        }
#endif

#if defined(LE_HOST)
#define GETHDRHG2FUNCUNITEXPR(_dt, _name, _expr)                                 \
        static uint32 ep_to_cpu_hdr##_dt##_##_name##_get(int unit, void *hdr) {  \
                ep_to_cpu_hdr##_dt##_t *hd = (ep_to_cpu_hdr##_dt##_t *)hdr;      \
                uint32  hgh[4];                                                  \
                soc_higig2_hdr_t *h = (soc_higig2_hdr_t *)&hgh[0];               \
                COMPILER_REFERENCE(unit);                                        \
                hgh[0] = soc_htonl(hd->mh0);                                     \
                hgh[1] = soc_htonl(hd->mh1);                                     \
                hgh[2] = soc_htonl(hd->mh2);                                     \
                hgh[3] = soc_htonl(hd->mh3);                                     \
                return _expr;                                                    \
        }
#else
#define GETHDRHG2FUNCUNITEXPR(_dt, _name, _expr)                                 \
        static uint32 ep_to_cpu_hdr##_dt##_##_name##_get(int unit, void *hdr) {  \
                ep_to_cpu_hdr##_dt##_t *hd = (ep_to_cpu_hdr##_dt##_t *)hdr;      \
                soc_higig2_hdr_t *h = (soc_higig2_hdr_t *)&hd->mh0;              \
                COMPILER_REFERENCE(unit);                                        \
                return _expr;                                                    \
        }
#endif

/*
 * Short cuts for generating dcb support functions.
 * Most support functions are just setting or getting a field
 * in the appropriate dcb structure or doing a simple expression
 * based on a couple of fields.
 *      GETFUNCFIELD - get a field from DCB
 *      SETFUNCFIELD - set a field in DCB
 *      SETFUNCERR - dummy handler for field that does not exist for
 *                      a descriptor type
 *      GETFUNCERR - dummy handler for field that does not exist for
 *                      a descriptor type
 */
#define GETFUNCEXPR(_dt, _name, _expr)                                  \
        static uint32 dcb##_dt##_##_name##_get(dcb_t *dcb) {            \
                dcb##_dt##_t *d = (dcb##_dt##_t *)dcb;                  \
                return _expr;                                           \
        }
#define GETFUNCFIELD(_dt, _name, _field)                                \
        GETFUNCEXPR(_dt, _name, d->_field)
#define GETFUNCERR(_dt, _name)                                          \
        static uint32 dcb##_dt##_##_name##_get(dcb_t *dcb) {            \
                COMPILER_REFERENCE(dcb);                                \
                dcb0_funcerr(_dt, #_name "_get");                       \
                return 0;                                               \
        }
#define GETFUNCNULL(_dt, _name)                                         \
        static uint32 dcb##_dt##_##_name##_get(dcb_t *dcb) {            \
                COMPILER_REFERENCE(dcb);                                \
                return 0;                                               \
        }
#define GETPTREXPR(_dt, _name, _expr)                                   \
        static uint32 * dcb##_dt##_##_name##_get(dcb_t *dcb) {          \
                dcb##_dt##_t *d = (dcb##_dt##_t *)dcb;                  \
                return _expr;                                           \
        }
#define GETPTRERR(_dt, _name)                                           \
        static uint32 * dcb##_dt##_##_name##_get(dcb_t *dcb) {          \
                COMPILER_REFERENCE(dcb);                                \
                dcb0_funcerr(_dt, #_name "_get");                       \
                return NULL;                                            \
        }
#define GETFUNCUNITEXPR(_dt, _name, _expr)                              \
        static uint32 dcb##_dt##_##_name##_get(int unit, dcb_t *dcb) {  \
                dcb##_dt##_t *d = (dcb##_dt##_t *)dcb;                  \
                COMPILER_REFERENCE(unit);                               \
                return _expr;                                           \
        }
#define GETFUNCUNITFIELD(_dt, _name, _field)                            \
        GETFUNCUNITEXPR(_dt, _name, d->_field)
#define GETFUNCUNITERR(_dt, _name)                                      \
        static uint32 dcb##_dt##_##_name##_get(int unit, dcb_t *dcb) {  \
                COMPILER_REFERENCE(unit);                               \
                COMPILER_REFERENCE(dcb);                                \
                dcb0_funcerr(_dt, #_name "_get");                       \
                return 0;                                               \
        }
#define SETFUNCEXPR(_dt, _name, _arg, _expr)                            \
        static void dcb##_dt##_##_name##_set(dcb_t *dcb, _arg) {        \
                dcb##_dt##_t *d = (dcb##_dt##_t *)dcb;                  \
                _expr;                                                  \
        }
#define SETFUNCFIELD(_dt, _name, _field, _arg, _expr)                   \
        SETFUNCEXPR(_dt, _name, _arg, d->_field = _expr)
#define SETFUNCERR(_dt, _name, _type)                                   \
        static void dcb##_dt##_##_name##_set(dcb_t *dcb, _type val) {   \
                COMPILER_REFERENCE(dcb);                                \
                COMPILER_REFERENCE(val);                                \
                dcb0_funcerr(_dt, #_name "_set");                       \
        }
#define SETFUNCNULL(_dt, _name)                                         \
        static uint32 dcb##_dt##_##_name##_set(dcb_t *dcb,uint32 val) { \
                COMPILER_REFERENCE(dcb);                                \
                return 0;                                               \
        }
#define SETFUNCNULL2(_dt, _name)                                        \
        static void dcb##_dt##_##_name##_set(dcb_t *dcb,uint32 val) {   \
                COMPILER_REFERENCE(dcb);                                \
        }

#define SETFUNCEXPRIGNORE(_dt, _name, _arg, _expr)                      \
        SETFUNCEXPR(_dt, _name, _arg, COMPILER_REFERENCE(d))
#if defined(LE_HOST)
#define GETHGFUNCEXPR(_dt, _name, _expr)                                \
        static uint32 dcb##_dt##_##_name##_get(dcb_t *dcb) {            \
                dcb##_dt##_t *d = (dcb##_dt##_t *)dcb;                  \
                uint32  hgh[3];                                         \
                soc_higig_hdr_t *h = (soc_higig_hdr_t *)&hgh[0];        \
                hgh[0] = soc_htonl(d->mh0);                             \
                hgh[1] = soc_htonl(d->mh1);                             \
                hgh[2] = soc_htonl(d->mh2);                             \
                return _expr;                                           \
        }
#else
#define GETHGFUNCEXPR(_dt, _name, _expr)                                \
        static uint32 dcb##_dt##_##_name##_get(dcb_t *dcb) {            \
                dcb##_dt##_t *d = (dcb##_dt##_t *)dcb;                  \
                soc_higig_hdr_t *h = (soc_higig_hdr_t *)&d->mh0;        \
                return _expr;                                           \
        }
#endif
#define GETHGFUNCFIELD(_dt, _name, _field)                              \
        GETHGFUNCEXPR(_dt, _name, h->hgp_overlay1._field)

#if defined(LE_HOST)
#define GETHG2FUNCEXPR(_dt, _name, _expr)                               \
        static uint32 dcb##_dt##_##_name##_get(dcb_t *dcb) {            \
                dcb##_dt##_t *d = (dcb##_dt##_t *)dcb;                  \
                uint32  hgh[4];                                         \
                soc_higig2_hdr_t *h = (soc_higig2_hdr_t *)&hgh[0];      \
                hgh[0] = soc_htonl(d->mh0);                             \
                hgh[1] = soc_htonl(d->mh1);                             \
                hgh[2] = soc_htonl(d->mh2);                             \
                hgh[3] = soc_htonl(d->mh3);                             \
                return _expr;                                           \
        }
#else
#define GETHG2FUNCEXPR(_dt, _name, _expr)                               \
        static uint32 dcb##_dt##_##_name##_get(dcb_t *dcb) {            \
                dcb##_dt##_t *d = (dcb##_dt##_t *)dcb;                  \
                soc_higig2_hdr_t *h = (soc_higig2_hdr_t *)&d->mh0;      \
                return _expr;                                           \
        }
#endif
#define GETHG2FUNCFIELD(_dt, _name, _field)                             \
        GETHG2FUNCEXPR(_dt, _name, h->ppd_overlay1._field)

#if defined(LE_HOST)
#define GETHGFUNCUNITEXPR(_dt, _name, _expr)                           \
        static uint32 dcb##_dt##_##_name##_get(int unit, dcb_t *dcb) {  \
                dcb##_dt##_t *d = (dcb##_dt##_t *)dcb;                  \
                uint32  hgh[3];                                         \
                soc_higig_hdr_t *h = (soc_higig_hdr_t *)&hgh[0];      \
                COMPILER_REFERENCE(unit);                               \
                hgh[0] = soc_htonl(d->mh0);                             \
                hgh[1] = soc_htonl(d->mh1);                             \
                hgh[2] = soc_htonl(d->mh2);                             \
                return _expr;                                           \
        }
#else
#define GETHGFUNCUNITEXPR(_dt, _name, _expr)                           \
        static uint32 dcb##_dt##_##_name##_get(int unit, dcb_t *dcb) {  \
                dcb##_dt##_t *d = (dcb##_dt##_t *)dcb;                  \
                soc_higig_hdr_t *h = (soc_higig_hdr_t *)&d->mh0;      \
                COMPILER_REFERENCE(unit);                               \
                return _expr;                                           \
        }
#endif

#if defined(LE_HOST)
#define GETHG2FUNCUNITEXPR(_dt, _name, _expr)                           \
        static uint32 dcb##_dt##_##_name##_get(int unit, dcb_t *dcb) {  \
                dcb##_dt##_t *d = (dcb##_dt##_t *)dcb;                  \
                uint32  hgh[4];                                         \
                soc_higig2_hdr_t *h = (soc_higig2_hdr_t *)&hgh[0];      \
                COMPILER_REFERENCE(unit);                               \
                hgh[0] = soc_htonl(d->mh0);                             \
                hgh[1] = soc_htonl(d->mh1);                             \
                hgh[2] = soc_htonl(d->mh2);                             \
                hgh[3] = soc_htonl(d->mh3);                             \
                return _expr;                                           \
        }
#else
#define GETHG2FUNCUNITEXPR(_dt, _name, _expr)                           \
        static uint32 dcb##_dt##_##_name##_get(int unit, dcb_t *dcb) {  \
                dcb##_dt##_t *d = (dcb##_dt##_t *)dcb;                  \
                soc_higig2_hdr_t *h = (soc_higig2_hdr_t *)&d->mh0;      \
                COMPILER_REFERENCE(unit);                               \
                return _expr;                                           \
        }
#endif

/*
 * This is a standard function used to generate a debug message whenever 
 * the code tries to access a field not present in the specific DCB
 */
static void
dcb0_funcerr(int dt, char *name)
{
    LOG_ERROR(BSL_LS_SOC_COMMON,
              (BSL_META("ERROR: dcb%d_%s called\n"), dt, name));
}

/* the addr related functions are the same for all dcb types */
static void
dcb0_addr_set(int unit, dcb_t *dcb, sal_vaddr_t addr)
{
    uint32      *d = (uint32 *)dcb;

    if (addr == 0) {
        *d = 0;
    } else {
        *d = soc_cm_l2p(unit, (void *)addr);
    }
}

static sal_vaddr_t
dcb0_addr_get(int unit, dcb_t *dcb)
{
    uint32      *d = (uint32 *)dcb;

    if (*d == 0) {
        return (sal_vaddr_t)0;
    } else {
        return (sal_vaddr_t)soc_cm_p2l(unit, *d);
    }
}

static sal_paddr_t
dcb0_paddr_get(dcb_t *dcb)
{
    uint32      *d = (uint32 *)dcb;

    return (sal_paddr_t)*d;
}

/*
 * Function:
 *      dcb0_rx_reason_map_get
 * Purpose:
 *      Return the RX reason map for a series of DCB types.
 * Parameters:
 *      dcb_op - DCB operations
 *      dcb    - dma control block
 * Returns:
 *      RX reason map pointer
 * Notes:
 *      Function made global to resolve compiler link issue.
 */
static soc_rx_reason_t *
dcb0_rx_reason_map_get(dcb_op_t *dcb_op, dcb_t *dcb)
{
    COMPILER_REFERENCE(dcb);

    return dcb_op->rx_reason_maps[0];
}

/*
 * Function:
 *      dcb0_rx_reasons_get
 * Purpose:
 *      Map the hardware reason bits from 'dcb' into the set
 *      of "reasons".
 * Parameters:
 *      dcb_op  - DCB operations
 *      dcb     - dma control block
 *      reasons - set of "reasons", socRxReason*
 */
static void        
dcb0_rx_reasons_get(dcb_op_t *dcb_op, dcb_t *dcb, soc_rx_reasons_t *reasons)
{
    soc_rx_reason_t *map;
    uint32 reason;
    uint32 mask;
    int i;    

    SOC_RX_REASON_CLEAR_ALL(*reasons);

    map = dcb_op->rx_reason_map_get(dcb_op, dcb);
    if (map == NULL) {
        return;
    }

    reason = dcb_op->rx_reason_get(dcb);
    mask = 1;
    for (i = 0; i < 32; i++) {
        if ((mask & reason)) {
            SOC_RX_REASON_SET(*reasons, map[i]);
        }
        mask <<= 1;
    }

    reason = dcb_op->rx_reason_hi_get(dcb);
    mask = 1;
    for (i = 0; i < 32; i++) {
        if ((mask & reason)) {
            SOC_RX_REASON_SET(*reasons, map[i + 32]);
        }
        mask <<= 1;
    }

    /* BPDU bit should be a reason, paste it in here */
    if (dcb_op->rx_bpdu_get(dcb)) {
        SOC_RX_REASON_SET(*reasons, socRxReasonBpdu);
    }

    return;
}

#if defined(BCM_XGS3_SWITCH_SUPPORT)
/*
 * DCB Type 9 Support
 */
GETFUNCFIELD(9, rx_l3_intf, l3_intf)
#endif /* BCM_XGS3_SWITCH_SUPPORT */

#if defined(BCM_TRIUMPH2_SUPPORT)
static void
dcb19_init(dcb_t *dcb)
{
    uint32      *d = (uint32 *)dcb;

    d[0] = d[1] = d[2] = d[3] = d[4] = 0;
    d[5] = d[6] = d[7] = d[8] = d[9] = d[10] = 0;
    d[11] = d[12] = d[13] = d[14] = d[15] = 0;
}

static int
dcb19_addtx(dv_t *dv, sal_vaddr_t addr, uint32 count,
            pbmp_t l2pbm, pbmp_t utpbm, pbmp_t l3pbm, uint32 flags, uint32 *hgh)
{
    dcb19_t     *d;     /* DCB */
    uint32      *di;    /* DCB integer pointer */
    uint32      paddr;  /* Packet buffer physical address */
    int         unaligned;
    int         unaligned_bytes;
    uint8       *unaligned_buffer;
    uint8       *aligned_buffer;

    d = (dcb19_t *)SOC_DCB_IDX2PTR(dv->dv_unit, dv->dv_dcb, dv->dv_vcnt);

    if (addr) {
        paddr = soc_cm_l2p(dv->dv_unit, (void *)addr);
    } else {
        paddr = 0;
    }

    if (dv->dv_vcnt > 0 && (dv->dv_flags & DV_F_COMBINE_DCB) &&
        (d[-1].c_sg != 0) &&
        (d[-1].addr + d[-1].c_count) == paddr &&
        d[-1].c_count + count <= DCB_MAX_REQCOUNT) {
        d[-1].c_count += count;
        return dv->dv_cnt - dv->dv_vcnt;
    }

    /*
     * A few chip revisions do not support 128 byte PCI bursts
     * correctly if the address is not word-aligned. In case
     * we encounter an unaligned address, we consume an extra
     * DCB to correct the alignment.
     */
    do {
        if (dv->dv_vcnt >= dv->dv_cnt) {
            return SOC_E_FULL;
        }
        if (dv->dv_vcnt > 0) {      /* chain off previous dcb */
            d[-1].c_chain = 1;
        }

        di = (uint32 *)d;
        di[0] = di[1] = di[2] = di[3] = di[4] = 0;
        di[5] = di[6] = di[7] = di[8] = di[9] = di[10] = 0;
        di[11] = di[12] = di[13] = di[14] = di[15] = 0;

        d->addr = paddr;
        d->c_count = count;
        d->c_sg = 1;

        d->c_stat = 1;
        d->c_purge = SOC_DMA_PURGE_GET(flags);
        if (SOC_DMA_HG_GET(flags)) {
            soc_higig_hdr_t *mh = (soc_higig_hdr_t *)hgh;
            if (mh->overlay1.start == SOC_HIGIG2_START) {
                d->mh3 = soc_ntohl(hgh[3]);
            }
            d->c_hg = 1;
            d->mh0 = soc_ntohl(hgh[0]);
            d->mh1 = soc_ntohl(hgh[1]);
            d->mh2 = soc_ntohl(hgh[2]);
            d->mh3 = soc_ntohl(hgh[3]);
        }

        unaligned = 0;
        if (soc_feature(dv->dv_unit, soc_feature_pkt_tx_align)) {
            if (paddr & 0x3) {
                unaligned_bytes = 4 - (paddr & 0x3);
                unaligned_buffer = (uint8 *)addr;
                aligned_buffer = SOC_DV_TX_ALIGN(dv, dv->dv_vcnt);
                aligned_buffer[0] = unaligned_buffer[0];
                aligned_buffer[1] = unaligned_buffer[1];
                aligned_buffer[2] = unaligned_buffer[2];
                d->addr = soc_cm_l2p(dv->dv_unit, aligned_buffer);
                if (count > 3) {
                    d->c_count = unaligned_bytes;
                    paddr += unaligned_bytes;
                    count -= unaligned_bytes;
                    unaligned = 1;
                }
            }
        }

        dv->dv_vcnt += 1;

        d = (dcb19_t *)SOC_DCB_IDX2PTR(dv->dv_unit, dv->dv_dcb, dv->dv_vcnt);

    } while (unaligned);

    return dv->dv_cnt - dv->dv_vcnt;
}

static int
dcb19_addrx(dv_t *dv, sal_vaddr_t addr, uint32 count, uint32 flags)
{
    dcb19_t     *d;     /* DCB */
    uint32      *di;    /* DCB integer pointer */

    d = (dcb19_t *)SOC_DCB_IDX2PTR(dv->dv_unit, dv->dv_dcb, dv->dv_vcnt);

    if (dv->dv_vcnt > 0) {      /* chain off previous dcb */
        d[-1].c_chain = 1;
    }

    di = (uint32 *)d;
    di[0] = di[1] = di[2] = di[3] = di[4] = 0;
    di[5] = di[6] = di[7] = di[8] = di[9] = di[10] = 0;
    di[11] = di[12] = di[13] = di[14] = di[15] = 0;

    if (addr) {
        d->addr = soc_cm_l2p(dv->dv_unit, (void *)addr);
    }
    d->c_count = count;
    d->c_sg = 1;

    dv->dv_vcnt += 1;
    return dv->dv_cnt - dv->dv_vcnt;
}

static uint32
dcb19_intrinfo(int unit, dcb_t *dcb, int tx, uint32 *count)
{
    dcb19_t      *d = (dcb19_t *)dcb;     /*  DCB */
    uint32      f;                      /* SOC_DCB_INFO_* flags */

    if (!d->done) {
        return 0;
    }
    f = SOC_DCB_INFO_DONE;
    if (tx) {
        if (!d->c_sg) {
            f |= SOC_DCB_INFO_PKTEND;
        }
    } else {
        if (d->end) {
            f |= SOC_DCB_INFO_PKTEND;
        }
    }
    *count = d->count;
    return f;
}

static uint32
dcb19_rx_untagged_get(dcb_t *dcb, int dt_mode, int ingport_is_hg)
{
    dcb19_t *d = (dcb19_t *)dcb;

    COMPILER_REFERENCE(dt_mode);

    return (ingport_is_hg ?
            ((d->itag_status) ? 0 : 2) :
            ((d->itag_status & 0x2) ?
             ((d->itag_status & 0x1) ? 0 : 2) :
             ((d->itag_status & 0x1) ? 1 : 3)));
}

SETFUNCFIELD(19, reqcount, c_count, uint32 count, count)
GETFUNCFIELD(19, reqcount, c_count)
GETFUNCFIELD(19, xfercount, count)
/* addr_set, addr_get, paddr_get - Same as DCB 0 */
SETFUNCFIELD(19, done, done, int val, val ? 1 : 0)
GETFUNCFIELD(19, done, done)
SETFUNCFIELD(19, sg, c_sg, int val, val ? 1 : 0)
GETFUNCFIELD(19, sg, c_sg)
SETFUNCFIELD(19, chain, c_chain, int val, val ? 1 : 0)
GETFUNCFIELD(19, chain, c_chain)
SETFUNCFIELD(19, reload, c_reload, int val, val ? 1 : 0)
GETFUNCFIELD(19, reload, c_reload)
SETFUNCERR(19, desc_intr, int)
GETFUNCERR(19, desc_intr)
SETFUNCERR(19, tx_l2pbm, pbmp_t)
SETFUNCERR(19, tx_utpbm, pbmp_t)
SETFUNCERR(19, tx_l3pbm, pbmp_t)
SETFUNCERR(19, tx_crc, int)
SETFUNCERR(19, tx_cos, int)
SETFUNCERR(19, tx_destmod, uint32)
SETFUNCERR(19, tx_destport, uint32)
SETFUNCERR(19, tx_opcode, uint32)
SETFUNCERR(19, tx_srcmod, uint32)
SETFUNCERR(19, tx_srcport, uint32)
SETFUNCERR(19, tx_prio, uint32)
SETFUNCERR(19, tx_pfm, uint32)
GETFUNCFIELD(19, rx_start, start)
GETFUNCFIELD(19, rx_end, end)
GETFUNCFIELD(19, rx_error, error)
GETFUNCFIELD(19, rx_cos, cpu_cos)
/* Fields extracted from MH/PBI */
GETHG2FUNCFIELD(19, rx_destmod, dst_mod)
GETHG2FUNCFIELD(19, rx_destport, dst_port)
GETHG2FUNCFIELD(19, rx_srcmod, src_mod)
GETHG2FUNCFIELD(19, rx_srcport, src_port)
GETHG2FUNCFIELD(19, rx_opcode, opcode)
GETHG2FUNCFIELD(19, rx_prio, vlan_pri) /* outer_pri */
GETHG2FUNCEXPR(19, rx_mcast, ((h->ppd_overlay1.dst_mod << 8) |
                              (h->ppd_overlay1.dst_port)))
GETHG2FUNCEXPR(19, rx_vclabel, ((h->ppd_overlay1.vc_label_19_16 << 16) |
                              (h->ppd_overlay1.vc_label_15_8 << 8) |
                              (h->ppd_overlay1.vc_label_7_0)))
GETHG2FUNCEXPR(19, rx_classtag, (h->ppd_overlay1.ppd_type != 1 ? 0 :
                                 (h->ppd_overlay2.ctag_hi << 8) |
                                 (h->ppd_overlay2.ctag_lo)))
GETFUNCFIELD(19, rx_reason, reason)
GETFUNCFIELD(19, rx_reason_hi, reason_hi)
GETFUNCFIELD(19, rx_ingport, srcport)
GETFUNCEXPR(19, rx_mirror, ((d->imirror) | (d->emirror)))
SETFUNCFIELD(19, hg, c_hg, uint32 hg, hg)
GETFUNCFIELD(19, hg, c_hg)
SETFUNCFIELD(19, stat, c_stat, uint32 stat, stat)
GETFUNCFIELD(19, stat, c_stat)
SETFUNCFIELD(19, purge, c_purge, uint32 purge, purge)
GETFUNCFIELD(19, purge, c_purge)
GETPTREXPR(19, mhp, &(d->mh0))
GETFUNCFIELD(19, outer_vid, outer_vid)
GETFUNCFIELD(19, outer_pri, outer_pri)
GETFUNCFIELD(19, outer_cfi, outer_cfi)
GETFUNCFIELD(19, rx_outer_tag_action, otag_action)
GETFUNCFIELD(19, inner_vid, inner_vid)
GETFUNCFIELD(19, inner_pri, inner_pri)
GETFUNCFIELD(19, inner_cfi, inner_cfi)
GETFUNCFIELD(19, rx_inner_tag_action, itag_action)
GETFUNCFIELD(19, rx_bpdu, bpdu)
GETFUNCNULL(19, rx_decap_tunnel)

static uint32 dcb19_rx_crc_get(dcb_t *dcb) {
    return 0;
}

#endif /* BCM_TRIUMPH2_SUPPORT */

#if defined(BCM_ENDURO_SUPPORT) || defined(BCM_HURRICANE_SUPPORT)
/*
 * DCB Type 20 Support
 */

static soc_rx_reason_t
dcb20_rx_reason_map[] = {
    socRxReasonUnknownVlan,        /* Offset 0 */
    socRxReasonL2SourceMiss,       /* Offset 1 */
    socRxReasonL2DestMiss,         /* Offset 2 */
    socRxReasonL2Move,             /* Offset 3 */
    socRxReasonL2Cpu,              /* Offset 4 */
    socRxReasonSampleSource,       /* Offset 5 */
    socRxReasonSampleDest,         /* Offset 6 */
    socRxReasonL3SourceMiss,       /* Offset 7 */
    socRxReasonL3DestMiss,         /* Offset 8 */
    socRxReasonL3SourceMove,       /* Offset 9 */
    socRxReasonMcastMiss,          /* Offset 10 */
    socRxReasonIpMcastMiss,        /* Offset 11 */
    socRxReasonFilterMatch,        /* Offset 12 */
    socRxReasonL3HeaderError,      /* Offset 13 */
    socRxReasonProtocol,           /* Offset 14 */
    socRxReasonDosAttack,          /* Offset 15 */
    socRxReasonMartianAddr,        /* Offset 16 */
    socRxReasonTunnelError,        /* Offset 17 */
    socRxReasonL2MtuFail,          /* Offset 18 */
    socRxReasonIcmpRedirect,       /* Offset 19 */
    socRxReasonL3Slowpath,         /* Offset 20 */
    socRxReasonParityError,        /* Offset 21 */
    socRxReasonL3MtuFail,          /* Offset 22 */
    socRxReasonHigigHdrError,      /* Offset 23 */
    socRxReasonMcastIdxError,      /* Offset 24 */
    socRxReasonVlanFilterMatch,    /* Offset 25 */
    socRxReasonClassBasedMove,     /* Offset 26 */
    socRxReasonL2LearnLimit,       /* Offset 27 */
    socRxReasonMplsLabelMiss,      /* Offset 28 */
    socRxReasonMplsInvalidAction,  /* Offset 29 */
    socRxReasonMplsInvalidPayload, /* Offset 30 */
    socRxReasonMplsTtl,            /* Offset 31 */
    socRxReasonMplsSequenceNumber, /* Offset 32 */
    socRxReasonL2NonUnicastMiss,   /* Offset 33 */
    socRxReasonNhop,               /* Offset 34 */
    socRxReasonMplsCtrlWordError,  /* Offset 35 */
    socRxReasonTimeSync,           /* Offset 36 */
    socRxReasonOAMSlowpath,        /* Offset 37 */
    socRxReasonOAMError,           /* Offset 38 */
    socRxReasonOAMLMDM,            /* Offset 39 */
    socRxReasonInvalid,            /* Offset 40 */
    socRxReasonInvalid,            /* Offset 41 */
    socRxReasonInvalid,            /* Offset 42 */
    socRxReasonInvalid,            /* Offset 43 */
    socRxReasonInvalid,            /* Offset 44 */
    socRxReasonInvalid,            /* Offset 45 */
    socRxReasonInvalid,            /* Offset 46 */
    socRxReasonInvalid,            /* Offset 47 */
    socRxReasonInvalid,            /* Offset 48 */
    socRxReasonInvalid,            /* Offset 49 */
    socRxReasonInvalid,            /* Offset 50 */
    socRxReasonInvalid,            /* Offset 51 */
    socRxReasonInvalid,            /* Offset 52 */
    socRxReasonInvalid,            /* Offset 53 */
    socRxReasonInvalid,            /* Offset 54 */
    socRxReasonInvalid,            /* Offset 55 */
    socRxReasonInvalid,            /* Offset 56 */
    socRxReasonInvalid,            /* Offset 57 */
    socRxReasonInvalid,            /* Offset 58 */
    socRxReasonInvalid,            /* Offset 59 */
    socRxReasonInvalid,            /* Offset 60 */
    socRxReasonInvalid,            /* Offset 61 */
    socRxReasonInvalid,            /* Offset 62 */
    socRxReasonInvalid             /* Offset 63 */
};

static soc_rx_reason_t *dcb20_rx_reason_maps[] = {
    dcb20_rx_reason_map,
    NULL
};

GETFUNCFIELD(20, rx_matchrule, match_rule)
GETFUNCFIELD(20, rx_timestamp, timestamp)
GETFUNCFIELD(20, rx_timestamp_upper, timestamp_upper)
#endif /* BCM_ENDURO_SUPPORT || BCM_HURRICANE_SUPPORT */

#if defined(BCM_TRIDENT_SUPPORT)
/*
 * DCB Type 21 Support
 */
/* From FORMAT NIV_CPU_OPCODE_ENCODING */
static soc_rx_reason_t dcb21_rx_reason_niv_encoding_map[] = {
    socRxReasonNiv,                /* 0: NO_ERRORS
                                    * Base field, must match the entries above */
    socRxReasonNivPrioDrop,        /* 1:DOT1P_ADMITTANCE_DISCARD */
    socRxReasonNivInterfaceMiss,   /* 2:VIF_LOOKUP_MISS */
    socRxReasonNivRpfFail,         /* 3:RPF_LOOKUP_MISS */
    socRxReasonNivTagInvalid,      /* 4:VNTAG_FORMAT_ERROR */
    socRxReasonNivTagDrop,         /* 5:VNTAG_PRESENT_DROP */
    socRxReasonNivUntagDrop,       /* 6:VNTAG_NOT_PRESENT_DROP */
    socRxReasonInvalid,            /* Offset 7 */
    socRxReasonInvalid,            /* Offset 8 */
    socRxReasonInvalid,            /* Offset 9 */
    socRxReasonInvalid,            /* Offset 10 */
    socRxReasonInvalid,            /* Offset 11 */
    socRxReasonInvalid,            /* Offset 12 */
    socRxReasonInvalid,            /* Offset 13 */
    socRxReasonInvalid,            /* Offset 14 */
    socRxReasonInvalid,            /* Offset 15 */
    socRxReasonInvalid,            /* Offset 16 */
    socRxReasonInvalid,            /* Offset 17 */
    socRxReasonInvalid,            /* Offset 18 */
    socRxReasonInvalid,            /* Offset 19 */
    socRxReasonInvalid,            /* Offset 20 */
    socRxReasonInvalid,            /* Offset 21 */
    socRxReasonInvalid,            /* Offset 22 */
    socRxReasonInvalid,            /* Offset 23 */
    socRxReasonInvalid,            /* Offset 24 */
    socRxReasonInvalid,            /* Offset 25 */
    socRxReasonInvalid,            /* Offset 26 */
    socRxReasonInvalid,            /* Offset 27 */
    socRxReasonInvalid,            /* Offset 28 */
    socRxReasonInvalid,            /* Offset 29 */
    socRxReasonInvalid,            /* Offset 30 */
    socRxReasonInvalid,            /* Offset 31 */
    socRxReasonInvalid,            /* Offset 32 */
    socRxReasonInvalid,            /* Offset 33 */
    socRxReasonInvalid,            /* Offset 34 */
    socRxReasonInvalid,            /* Offset 35 */
    socRxReasonInvalid,            /* Offset 36 */
    socRxReasonInvalid,            /* Offset 37 */
    socRxReasonInvalid,            /* Offset 38 */
    socRxReasonInvalid,            /* Offset 39 */
    socRxReasonInvalid,            /* Offset 40 */
    socRxReasonInvalid,            /* Offset 41 */
    socRxReasonInvalid,            /* Offset 42 */
    socRxReasonInvalid,            /* Offset 43 */
    socRxReasonInvalid,            /* Offset 44 */
    socRxReasonInvalid,            /* Offset 45 */
    socRxReasonInvalid,            /* Offset 46 */
    socRxReasonInvalid,            /* Offset 47 */
    socRxReasonInvalid,            /* Offset 48 */
    socRxReasonInvalid,            /* Offset 49 */
    socRxReasonInvalid,            /* Offset 50 */
    socRxReasonInvalid,            /* Offset 51 */
    socRxReasonInvalid,            /* Offset 52 */
    socRxReasonInvalid,            /* Offset 53 */
    socRxReasonInvalid,            /* Offset 54 */
    socRxReasonInvalid,            /* Offset 55 */
    socRxReasonInvalid,            /* Offset 56 */
    socRxReasonInvalid,            /* Offset 57 */
    socRxReasonInvalid,            /* Offset 58 */
    socRxReasonInvalid,            /* Offset 59 */
    socRxReasonInvalid,            /* Offset 60 */
    socRxReasonInvalid,            /* Offset 61 */
    socRxReasonInvalid,            /* Offset 62 */
    socRxReasonInvalid             /* Offset 63 */
};

static void
dcb21_rx_reasons_get(dcb_op_t *dcb_op, dcb_t *dcb, soc_rx_reasons_t *reasons)
{
    soc_rx_reason_t *map, *encoding_map;
    uint32 opcode[2], encoding[3];
    uint32 bit_val;
    int word_idx, bit_idx, opcode_idx, map_idx, word_count;
    int enc_bit = 0;

    SOC_RX_REASON_CLEAR_ALL(*reasons);

    opcode[0] = dcb_op->rx_reason_get(dcb);
    opcode[1] = dcb_op->rx_reason_hi_get(dcb);
    word_count = 2;
    sal_memset(encoding, 0, sizeof(encoding));
    map = dcb_op->rx_reason_map_get(dcb_op, dcb);
    for (word_idx = 0; word_idx < word_count; word_idx++) {
        for (bit_idx = 0; bit_idx < 32; bit_idx++) {
            opcode_idx = word_idx * 32 + bit_idx;
            bit_val = opcode[word_idx] & (1 << bit_idx) ? 1 : 0;
            for (map_idx = 0; ; map_idx++) {
                encoding_map = dcb_op->rx_reason_maps[1 + map_idx];
                if (encoding_map == NULL) {
                    break;
                }
                if (map[opcode_idx] != encoding_map[0]) {
                    continue;
                }
                if (opcode_idx == 0 || map[opcode_idx - 1] != encoding_map[0]) {
                    enc_bit = 0;
                }
                encoding[map_idx] |= bit_val << enc_bit;
                enc_bit++;
                break;
            }
            if (encoding_map == NULL && bit_val) {
                SOC_RX_REASON_SET(*reasons, map[opcode_idx]);
            }
        }
    }

    for (map_idx = 0; ; map_idx++) {
        encoding_map = dcb_op->rx_reason_maps[1 + map_idx];
        if (encoding_map == NULL) {
            break;
        }
        if (encoding[map_idx] != socRxReasonInvalid) {
            SOC_RX_REASON_SET(*reasons, encoding_map[encoding[map_idx]]);
        }
    }

    /* BPDU bit should be a reason, paste it in here */
    if (dcb_op->rx_bpdu_get(dcb)) {
        SOC_RX_REASON_SET(*reasons, socRxReasonBpdu);
    }

    return;
}
#endif /* BCM_TRIDENT_SUPPORT */

#if defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_KATANA_SUPPORT) || \
    defined(BCM_TRIDENT2_SUPPORT) || defined(BCM_GREYHOUND_SUPPORT)
/*
 * DCB Type 23 Support
 */
static soc_rx_reason_t
dcb23_rx_reason_map_ip_0[] = { /* IP Overlay 0 */
    socRxReasonUnknownVlan,        /* Offset 0 */
    socRxReasonL2SourceMiss,       /* Offset 1 */
    socRxReasonL2DestMiss,         /* Offset 2 */
    socRxReasonL2Move,             /* Offset 3 */
    socRxReasonL2Cpu,              /* Offset 4 */
    socRxReasonSampleSource,       /* Offset 5 */
    socRxReasonSampleDest,         /* Offset 6 */
    socRxReasonL3SourceMiss,       /* Offset 7 */
    socRxReasonL3DestMiss,         /* Offset 8 */
    socRxReasonL3SourceMove,       /* Offset 9 */
    socRxReasonMcastMiss,          /* Offset 10 */
    socRxReasonIpMcastMiss,        /* Offset 11 */
    socRxReasonL3HeaderError,      /* Offset 12 */
    socRxReasonProtocol,           /* Offset 13 */
    socRxReasonDosAttack,          /* Offset 14 */
    socRxReasonMartianAddr,        /* Offset 15 */
    socRxReasonTunnelError,        /* Offset 16 */
    socRxReasonMirror,             /* Offset 17 */ 
    socRxReasonIcmpRedirect,       /* Offset 18 */
    socRxReasonL3Slowpath,         /* Offset 19 */
    socRxReasonL3MtuFail,          /* Offset 20 */
    socRxReasonMcastIdxError,      /* Offset 21 */
    socRxReasonVlanFilterMatch,    /* Offset 22 */
    socRxReasonClassBasedMove,     /* Offset 23 */
    socRxReasonL3AddrBindFail,     /* Offset 24 */
    socRxReasonMplsLabelMiss,      /* Offset 25 */
    socRxReasonMplsInvalidAction,  /* Offset 26 */
    socRxReasonMplsInvalidPayload, /* Offset 27 */
    socRxReasonMplsTtl,            /* Offset 28 */
    socRxReasonMplsSequenceNumber, /* Offset 29 */
    socRxReasonL2NonUnicastMiss,   /* Offset 30 */
    socRxReasonNhop,               /* Offset 31 */
    socRxReasonStation,            /* Offset 32 */
    socRxReasonVlanTranslate,      /* Offset 33 */
    socRxReasonTimeSync,           /* Offset 34 */
    socRxReasonOAMSlowpath,        /* Offset 35 */
    socRxReasonOAMError,           /* Offset 36 */
    socRxReasonIpfixRateViolation, /* Offset 37 */
    socRxReasonL2LearnLimit,       /* Offset 38 */
    socRxReasonEncapHigigError,    /* Offset 39 */
    socRxReasonRegexMatch,         /* Offset 40 */
    socRxReasonOAMLMDM,            /* Offset 41 */
    socRxReasonBfd,                /* Offset 42 */
    socRxReasonBfdSlowpath,        /* Offset 43 */
    socRxReasonFailoverDrop,       /* Offset 44 */
    socRxReasonTrillName,          /* Offset 45 */
    socRxReasonTrillTtl,           /* Offset 46 */
    socRxReasonTrillCoreIsIs,      /* Offset 47 */
    socRxReasonTrillSlowpath,      /* Offset 48 */
    socRxReasonTrillRpfFail,       /* Offset 49 */
    socRxReasonTrillMiss,          /* Offset 50 */
    socRxReasonTrillInvalid,       /* Offset 51 */
    socRxReasonNivUntagDrop,       /* Offset 52 */
    socRxReasonNivTagDrop,         /* Offset 53 */
    socRxReasonNivTagInvalid,      /* Offset 54 */
    socRxReasonNivRpfFail,         /* Offset 55 */
    socRxReasonNivInterfaceMiss,   /* Offset 56 */
    socRxReasonNivPrioDrop,        /* Offset 57 */
    socRxReasonParityError,        /* Offset 58 */
    socRxReasonHigigHdrError,      /* Offset 59 */
    socRxReasonFilterMatch,        /* Offset 60 */
    socRxReasonL2GreSipMiss,       /* Offset 61 */
    socRxReasonL2GreVpnIdMiss,     /* Offset 62 */
    socRxReasonInvalid             /* Offset 63 */
};

static soc_rx_reason_t
dcb23_rx_reason_map_ip_1[] = { /* IP Overlay 1 */
    socRxReasonUnknownVlan,        /* Offset 0 */
    socRxReasonL2SourceMiss,       /* Offset 1 */
    socRxReasonL2DestMiss,         /* Offset 2 */
    socRxReasonL2Move,             /* Offset 3 */
    socRxReasonL2Cpu,              /* Offset 4 */
    socRxReasonSampleSource,       /* Offset 5 */
    socRxReasonSampleDest,         /* Offset 6 */
    socRxReasonL3SourceMiss,       /* Offset 7 */
    socRxReasonL3DestMiss,         /* Offset 8 */
    socRxReasonL3SourceMove,       /* Offset 9 */
    socRxReasonMcastMiss,          /* Offset 10 */
    socRxReasonIpMcastMiss,        /* Offset 11 */
    socRxReasonL3HeaderError,      /* Offset 12 */
    socRxReasonProtocol,           /* Offset 13 */
    socRxReasonDosAttack,          /* Offset 14 */
    socRxReasonMartianAddr,        /* Offset 15 */
    socRxReasonTunnelError,        /* Offset 16 */
    socRxReasonMirror,             /* Offset 17 */ 
    socRxReasonIcmpRedirect,       /* Offset 18 */
    socRxReasonL3Slowpath,         /* Offset 19 */
    socRxReasonL3MtuFail,          /* Offset 20 */
    socRxReasonMcastIdxError,      /* Offset 21 */
    socRxReasonVlanFilterMatch,    /* Offset 22 */
    socRxReasonClassBasedMove,     /* Offset 23 */
    socRxReasonL3AddrBindFail,     /* Offset 24 */
    socRxReasonMplsLabelMiss,      /* Offset 25 */
    socRxReasonMplsInvalidAction,  /* Offset 26 */
    socRxReasonMplsInvalidPayload, /* Offset 27 */
    socRxReasonMplsTtl,            /* Offset 28 */
    socRxReasonMplsSequenceNumber, /* Offset 29 */
    socRxReasonL2NonUnicastMiss,   /* Offset 30 */
    socRxReasonNhop,               /* Offset 31 */
    socRxReasonStation,            /* Offset 32 */
    socRxReasonVlanTranslate,      /* Offset 33 */
    socRxReasonTimeSync,           /* Offset 34 */
    socRxReasonOAMSlowpath,        /* Offset 35 */
    socRxReasonOAMError,           /* Offset 36 */
    socRxReasonIpfixRateViolation, /* Offset 37 */
    socRxReasonL2LearnLimit,       /* Offset 38 */
    socRxReasonEncapHigigError,    /* Offset 39 */
    socRxReasonRegexMatch,         /* Offset 40 */
    socRxReasonOAMLMDM,            /* Offset 41 */
    socRxReasonBfd,                /* Offset 42 */
    socRxReasonBfdSlowpath,        /* Offset 43 */
    socRxReasonFailoverDrop,       /* Offset 44 */
    socRxReasonWlanSlowpathKeepalive, /* Offset 45 */
    socRxReasonWlanTunnelError,    /* Offset 46 */
    socRxReasonWlanSlowpath,       /* Offset 47 */
    socRxReasonWlanDot1xDrop,      /* Offset 48 */
    socRxReasonMplsReservedEntropyLabel, /* Offset 49 */
    socRxReasonCongestionCnmProxy, /* Offset 50 */
    socRxReasonCongestionCnmProxyError, /* Offset 51 */
    socRxReasonCongestionCnm,      /* Offset 52 */
    socRxReasonMplsUnknownAch,     /* Offset 53 */
    socRxReasonMplsLookupsExceeded, /* Offset 54 */
    socRxReasonMplsIllegalReservedLabel, /* Offset 55 */
    socRxReasonMplsRouterAlertLabel, /* Offset 56 */
    socRxReasonInvalid,            /* Offset 57 */
    socRxReasonParityError,        /* Offset 58 */
    socRxReasonHigigHdrError,      /* Offset 59 */
    socRxReasonFilterMatch,        /* Offset 60 */
    socRxReasonL2GreSipMiss,       /* Offset 61 */
    socRxReasonL2GreVpnIdMiss,     /* Offset 62 */
    socRxReasonInvalid             /* Offset 63 */
};

static soc_rx_reason_t
dcb23_rx_reason_map_ep[] = {
    socRxReasonUnknownVlan,        /* Offset 0 */
    socRxReasonStp,                /* Offset 1 */
    socRxReasonVlanTranslate,      /* Offset 2 new */
    socRxReasonTunnelError,        /* Offset 3 */
    socRxReasonIpmc,               /* Offset 4 */
    socRxReasonL3HeaderError,      /* Offset 5 */
    socRxReasonTtl,                /* Offset 6 */
    socRxReasonL2MtuFail,          /* Offset 7 */
    socRxReasonHigigHdrError,      /* Offset 8 */
    socRxReasonSplitHorizon,       /* Offset 9 */
    socRxReasonNivPrune,           /* Offset 10 */
    socRxReasonVirtualPortPrune,   /* Offset 11 */
    socRxReasonFilterMatch,        /* Offset 12 */
    socRxReasonNonUnicastDrop,     /* Offset 13 */
    socRxReasonTrillPacketPortMismatch, /* Offset 14 */
    socRxReasonInvalid,            /* Offset 15 */
    socRxReasonInvalid,            /* Offset 16 */
    socRxReasonInvalid,            /* Offset 17 */
    socRxReasonInvalid,            /* Offset 18 */
    socRxReasonInvalid,            /* Offset 19 */
    socRxReasonInvalid,            /* Offset 20 */
    socRxReasonInvalid,            /* Offset 21 */
    socRxReasonInvalid,            /* Offset 22 */
    socRxReasonInvalid,            /* Offset 23 */
    socRxReasonInvalid,            /* Offset 24 */
    socRxReasonInvalid,            /* Offset 25 */
    socRxReasonInvalid,            /* Offset 26 */
    socRxReasonInvalid,            /* Offset 27 */
    socRxReasonInvalid,            /* Offset 28 */
    socRxReasonInvalid,            /* Offset 29 */
    socRxReasonInvalid,            /* Offset 30 */
    socRxReasonInvalid,            /* Offset 31 */
    socRxReasonInvalid,            /* Offset 32 */
    socRxReasonInvalid,            /* Offset 33 */
    socRxReasonInvalid,            /* Offset 34 */
    socRxReasonInvalid,            /* Offset 35 */
    socRxReasonInvalid,            /* Offset 36 */
    socRxReasonInvalid,            /* Offset 37 */
    socRxReasonInvalid,            /* Offset 38 */
    socRxReasonInvalid,            /* Offset 39 */
    socRxReasonInvalid,            /* Offset 40 */
    socRxReasonInvalid,            /* Offset 41 */
    socRxReasonInvalid,            /* Offset 42 */
    socRxReasonInvalid,            /* Offset 43 */
    socRxReasonInvalid,            /* Offset 44 */
    socRxReasonInvalid,            /* Offset 45 */
    socRxReasonInvalid,            /* Offset 46 */
    socRxReasonInvalid,            /* Offset 47 */
    socRxReasonInvalid,            /* Offset 48 */
    socRxReasonInvalid,            /* Offset 49 */
    socRxReasonInvalid,            /* Offset 50 */
    socRxReasonInvalid,            /* Offset 51 */
    socRxReasonInvalid,            /* Offset 52 */
    socRxReasonInvalid,            /* Offset 53 */
    socRxReasonInvalid,            /* Offset 54 */
    socRxReasonInvalid,            /* Offset 55 */
    socRxReasonInvalid,            /* Offset 56 */
    socRxReasonInvalid,            /* Offset 57 */
    socRxReasonInvalid,            /* Offset 58 */
    socRxReasonInvalid,            /* Offset 59 */
    socRxReasonInvalid,            /* Offset 60 */
    socRxReasonInvalid,            /* Offset 61 */
    socRxReasonInvalid,            /* Offset 62 */
    socRxReasonInvalid             /* Offset 63 */
};

static soc_rx_reason_t
dcb23_rx_reason_map_nlf[] = {
    socRxReasonRegexAction,     /* Offset 0 */
    socRxReasonWlanClientMove,     /* Offset 1 */
    socRxReasonWlanSourcePortMiss, /* Offset 2 */
    socRxReasonWlanClientError,    /* Offset 3 */
    socRxReasonWlanClientSourceMiss, /* Offset 4 */
    socRxReasonWlanClientDestMiss, /* Offset 5 */
    socRxReasonWlanMtu,            /* Offset 6 */
    socRxReasonInvalid,            /* Offset 7 */
    socRxReasonInvalid,            /* Offset 8 */
    socRxReasonInvalid,            /* Offset 9 */
    socRxReasonInvalid,            /* Offset 10 */
    socRxReasonInvalid,            /* Offset 11 */
    socRxReasonInvalid,            /* Offset 12 */
    socRxReasonInvalid,            /* Offset 13 */
    socRxReasonInvalid,            /* Offset 14 */
    socRxReasonInvalid,            /* Offset 15 */
    socRxReasonInvalid,            /* Offset 16 */
    socRxReasonInvalid,            /* Offset 17 */
    socRxReasonInvalid,            /* Offset 18 */
    socRxReasonInvalid,            /* Offset 19 */
    socRxReasonInvalid,            /* Offset 20 */
    socRxReasonInvalid,            /* Offset 21 */
    socRxReasonInvalid,            /* Offset 22 */
    socRxReasonInvalid,            /* Offset 23 */
    socRxReasonInvalid,            /* Offset 24 */
    socRxReasonInvalid,            /* Offset 25 */
    socRxReasonInvalid,            /* Offset 26 */
    socRxReasonInvalid,            /* Offset 27 */
    socRxReasonInvalid,            /* Offset 28 */
    socRxReasonInvalid,            /* Offset 29 */
    socRxReasonInvalid,            /* Offset 30 */
    socRxReasonInvalid,            /* Offset 31 */
    socRxReasonInvalid,            /* Offset 32 */
    socRxReasonInvalid,            /* Offset 33 */
    socRxReasonInvalid,            /* Offset 34 */
    socRxReasonInvalid,            /* Offset 35 */
    socRxReasonInvalid,            /* Offset 36 */
    socRxReasonInvalid,            /* Offset 37 */
    socRxReasonInvalid,            /* Offset 38 */
    socRxReasonInvalid,            /* Offset 39 */
    socRxReasonInvalid,            /* Offset 40 */
    socRxReasonInvalid,            /* Offset 41 */
    socRxReasonInvalid,            /* Offset 42 */
    socRxReasonInvalid,            /* Offset 43 */
    socRxReasonInvalid,            /* Offset 44 */
    socRxReasonInvalid,            /* Offset 45 */
    socRxReasonInvalid,            /* Offset 46 */
    socRxReasonInvalid,            /* Offset 47 */
    socRxReasonInvalid,            /* Offset 48 */
    socRxReasonInvalid,            /* Offset 49 */
    socRxReasonInvalid,            /* Offset 50 */
    socRxReasonInvalid,            /* Offset 51 */
    socRxReasonInvalid,            /* Offset 52 */
    socRxReasonInvalid,            /* Offset 53 */
    socRxReasonInvalid,            /* Offset 54 */
    socRxReasonInvalid,            /* Offset 55 */
    socRxReasonInvalid,            /* Offset 56 */
    socRxReasonInvalid,            /* Offset 57 */
    socRxReasonInvalid,            /* Offset 58 */
    socRxReasonInvalid,            /* Offset 59 */
    socRxReasonInvalid,            /* Offset 60 */
    socRxReasonInvalid,            /* Offset 61 */
    socRxReasonInvalid,            /* Offset 62 */
    socRxReasonInvalid             /* Offset 63 */
};

static soc_rx_reason_t *dcb23_rx_reason_maps[] = {
    dcb23_rx_reason_map_ip_0,
    dcb23_rx_reason_map_ip_1,
    dcb23_rx_reason_map_ep,
    dcb23_rx_reason_map_nlf,
    NULL
};


/*
 * Function:
 *      dcb23_rx_reason_map_get
 * Purpose:
 *      Return the RX reason map for DCB 23 type.
 * Parameters:
 *      dcb_op - DCB operations
 *      dcb    - dma control block
 * Returns:
 *      RX reason map pointer
 */
static soc_rx_reason_t *
dcb23_rx_reason_map_get(dcb_op_t *dcb_op, dcb_t *dcb)
{
    soc_rx_reason_t *map = NULL;
    dcb23_t  *d = (dcb23_t *)dcb;

    switch (d->word4.overlay1.cpu_opcode_type) {
    case SOC_CPU_OPCODE_TYPE_IP_0:
        map = dcb23_rx_reason_map_ip_0;
        break;
    case SOC_CPU_OPCODE_TYPE_IP_1:
        map = dcb23_rx_reason_map_ip_1;
        break;
    case SOC_CPU_OPCODE_TYPE_EP:
        map = dcb23_rx_reason_map_ep;
        break;
    case SOC_CPU_OPCODE_TYPE_NLF:
        map = dcb23_rx_reason_map_nlf;
        break;
    default:
        /* Unknown reason type */
        break;
    }

    return map;
}

static _shr_rx_decap_tunnel_t dcb23_rx_decap_tunnel_map[] = {
    _SHR_RX_DECAP_NONE,
    _SHR_RX_DECAP_ACCESS_SVP,
    _SHR_RX_DECAP_MIM,
    _SHR_RX_DECAP_WTP2AC,
    _SHR_RX_DECAP_AC2AC,
    _SHR_RX_DECAP_AMT,
    _SHR_RX_DECAP_IP,
    _SHR_RX_DECAP_TRILL,
    _SHR_RX_DECAP_L2MPLS_1LABEL,
    _SHR_RX_DECAP_L2MPLS_1LABELCW,
    _SHR_RX_DECAP_L3MPLS_1LABEL,
    _SHR_RX_DECAP_L3MPLS_1LABELCW,
    _SHR_RX_DECAP_NONE,
    _SHR_RX_DECAP_L2GRE
}; 
static uint32 dcb23_rx_decap_tunnel_get(dcb_t *dcb) {
    dcb23_t *d = (dcb23_t *)dcb;
    uint32 decap_tunnel_type = d->word11.overlay1.decap_tunnel_type;
    /* Check for array bounds */
    if ( (decap_tunnel_type <  
    (sizeof (dcb23_rx_decap_tunnel_map) / sizeof (_shr_rx_decap_tunnel_t))) ) {
        return (uint32)dcb23_rx_decap_tunnel_map[decap_tunnel_type];
    }
    return (uint32)_SHR_RX_DECAP_NONE;
}

static uint32
dcb23_rx_untagged_get(dcb_t *dcb, int dt_mode, int ingport_is_hg)
{
    dcb23_t *d = (dcb23_t *)dcb;
    uint32 hgh[4];
    soc_higig2_hdr_t *h = (soc_higig2_hdr_t *)&hgh[0];
    COMPILER_REFERENCE(dt_mode);

    hgh[0] = soc_htonl(d->mh0);
    hgh[1] = soc_htonl(d->mh1);
    hgh[2] = soc_htonl(d->mh2);
    hgh[3] = soc_htonl(d->mh3);

   return ((ingport_is_hg &&
        ((h->ppd_overlay1.ppd_type == 0) ||
        (h->ppd_overlay1.ppd_type == 1))) ?
        ((d->itag_status) ? 0 : 2) :
        ((d->itag_status & 0x2) ?
        ((d->itag_status & 0x1) ? 0 : 2) :
        ((d->itag_status & 0x1) ? 1 : 3)));


}

GETFUNCFIELD(23, xfercount, count)
GETFUNCFIELD(23, rx_cos, word4.overlay1.queue_num)

/* Fields extracted from MH/PBI */
GETHG2FUNCFIELD(23, rx_destmod, dst_mod)
GETHG2FUNCFIELD(23, rx_destport, dst_port)
GETHG2FUNCFIELD(23, rx_srcmod, src_mod)
GETHG2FUNCFIELD(23, rx_srcport, src_port)
GETHG2FUNCFIELD(23, rx_opcode, opcode)
GETHG2FUNCFIELD(23, rx_prio, vlan_pri) /* outer_pri */
GETHG2FUNCEXPR(23, rx_mcast, ((h->ppd_overlay1.dst_mod << 8) |
                              (h->ppd_overlay1.dst_port)))
GETHG2FUNCEXPR(23, rx_vclabel, ((h->ppd_overlay1.vc_label_19_16 << 16) |
                              (h->ppd_overlay1.vc_label_15_8 << 8) |
                              (h->ppd_overlay1.vc_label_7_0)))
GETHG2FUNCEXPR(23, rx_classtag, (h->ppd_overlay1.ppd_type != 1 ? 0 :
                                 (h->ppd_overlay2.ctag_hi << 8) |
                                 (h->ppd_overlay2.ctag_lo)))
GETFUNCFIELD(23, rx_matchrule, match_rule)
GETFUNCFIELD(23, rx_reason, reason)
GETFUNCFIELD(23, rx_reason_hi, reason_hi)
GETFUNCFIELD(23, rx_ingport, srcport)
GETFUNCEXPR(23, rx_mirror, ((SOC_CPU_OPCODE_TYPE_IP_0 ==
                            d->word4.overlay1.cpu_opcode_type) ?
                            (d->reason & (1 << 17)) : 0))
GETFUNCFIELD(23, rx_timestamp, word12.overlay1.timestamp)
GETFUNCFIELD(23, rx_timestamp_upper, word14.overlay1.timestamp_hi)
GETPTREXPR(23, mhp, &(d->mh0))
GETFUNCFIELD(23, outer_vid, word4.overlay1.outer_vid)
GETFUNCFIELD(23, outer_pri, word11.overlay1.outer_pri)
GETFUNCFIELD(23, outer_cfi, word11.overlay1.outer_cfi)
GETFUNCFIELD(23, rx_outer_tag_action, otag_action)
GETFUNCFIELD(23, inner_vid, word11.overlay1.inner_vid)
GETFUNCFIELD(23, inner_pri, inner_pri)
GETFUNCFIELD(23, inner_cfi, word11.overlay1.inner_cfi)
GETFUNCFIELD(23, rx_inner_tag_action, itag_action)
GETFUNCFIELD(23, rx_bpdu, bpdu)
GETFUNCEXPR(23, rx_l3_intf, ((d->replicated) ? (d->repl_nhi) :
                (((d->repl_nhi) & 0x40000) ?  /* TR3 NHI */
                 (((d->repl_nhi) & 0x3ffff) + _SHR_L3_EGRESS_IDX_MIN) :
                 ((d->repl_nhi) & 0x4000) ?   /* HX4 NHI */
                 (((d->repl_nhi) & 0x3fff) + _SHR_L3_EGRESS_IDX_MIN) :
                 (d->repl_nhi))))
GETFUNCFIELD(23, rx_switch_drop, all_switch_drop)
GETFUNCNULL(23, olp_encap_oam_pkt)
GETFUNCNULL(23, read_ecc_error)
GETFUNCNULL(23, desc_remaining)
SETFUNCNULL(23, desc_remaining)
GETFUNCNULL(23, desc_status)
SETFUNCNULL(23, desc_status)

dcb_op_t dcb23_op = {
    23,
    sizeof(dcb23_t),
    dcb23_rx_reason_maps,
    dcb23_rx_reason_map_get,
    dcb0_rx_reasons_get,
    dcb19_init,
    dcb19_addtx,
    dcb19_addrx,
    dcb19_intrinfo,
    dcb19_reqcount_set,
    dcb19_reqcount_get,
    dcb23_xfercount_get,
    dcb0_addr_set,
    dcb0_addr_get,
    dcb0_paddr_get,
    dcb19_done_set,
    dcb19_done_get,
    dcb19_sg_set,
    dcb19_sg_get,
    dcb19_chain_set,
    dcb19_chain_get,
    dcb19_reload_set,
    dcb19_reload_get,
    dcb19_desc_intr_set,
    dcb19_desc_intr_get,
    dcb19_tx_l2pbm_set,
    dcb19_tx_utpbm_set,
    dcb19_tx_l3pbm_set,
    dcb19_tx_crc_set,
    dcb19_tx_cos_set,
    dcb19_tx_destmod_set,
    dcb19_tx_destport_set,
    dcb19_tx_opcode_set,
    dcb19_tx_srcmod_set,
    dcb19_tx_srcport_set,
    dcb19_tx_prio_set,
    dcb19_tx_pfm_set,
    dcb23_rx_untagged_get,
    dcb19_rx_crc_get,
    dcb23_rx_cos_get,
    dcb23_rx_destmod_get,
    dcb23_rx_destport_get,
    dcb23_rx_opcode_get,
    dcb23_rx_classtag_get,
    dcb23_rx_matchrule_get,
    dcb19_rx_start_get,
    dcb19_rx_end_get,
    dcb19_rx_error_get,
    dcb23_rx_prio_get,
    dcb23_rx_reason_get,
    dcb23_rx_reason_hi_get,
    dcb23_rx_ingport_get,
    dcb23_rx_srcport_get,
    dcb23_rx_srcmod_get,
    dcb23_rx_mcast_get,
    dcb23_rx_vclabel_get,
    dcb23_rx_mirror_get,
    dcb23_rx_timestamp_get,
    dcb23_rx_timestamp_upper_get,
    dcb19_hg_set,
    dcb19_hg_get,
    dcb19_stat_set,
    dcb19_stat_get,
    dcb19_purge_set,
    dcb19_purge_get,
    dcb23_mhp_get,
    dcb23_outer_vid_get,
    dcb23_outer_pri_get,
    dcb23_outer_cfi_get,
    dcb23_rx_outer_tag_action_get,
    dcb23_inner_vid_get,
    dcb23_inner_pri_get,
    dcb23_inner_cfi_get,
    dcb23_rx_inner_tag_action_get,
    dcb23_rx_bpdu_get,
    dcb23_rx_l3_intf_get,
    dcb23_rx_decap_tunnel_get,
    dcb23_rx_switch_drop_get,
    dcb23_olp_encap_oam_pkt_get,
    dcb23_read_ecc_error_get,
    dcb23_desc_remaining_get,
    dcb23_desc_remaining_set,
    dcb23_desc_status_get,
    dcb23_desc_status_set,
    NULL,
};
#endif /* BCM_TRIUMPH3_SUPPORT */

#if defined(BCM_TRIDENT2_SUPPORT)
/*
 * DCB Type 26 Support
 */
/* From FORMAT CPU_OPCODES */
static soc_rx_reason_t
dcb26_rx_reason_map[] = {
    socRxReasonUnknownVlan,        /*  0: CPU_UVLAN */
    socRxReasonL2SourceMiss,       /*  1: CPU_SLF */
    socRxReasonL2DestMiss,         /*  2: CPU_DLF */
    socRxReasonL2Move,             /*  3: CPU_L2MOVE */
    socRxReasonL2Cpu,              /*  4: CPU_L2CPU */
    socRxReasonSampleSource,       /*  5: CPU_SFLOW_SRC */
    socRxReasonSampleDest,         /*  6: CPU_SFLOW_DST */
    socRxReasonL3SourceMiss,       /*  7: CPU_L3SRC_MISS */
    socRxReasonL3DestMiss,         /*  8: CPU_L3DST_MISS */
    socRxReasonL3SourceMove,       /*  9: CPU_L3SRC_MOVE */
    socRxReasonMcastMiss,          /* 10: CPU_MC_MISS */
    socRxReasonIpMcastMiss,        /* 11: CPU_IPMC_MISS */
    socRxReasonFilterMatch,        /* 12: CPU_FFP */
    socRxReasonL3HeaderError,      /* 13: CPU_L3HDR_ERR */
    socRxReasonProtocol,           /* 14: CPU_PROTOCOL_PKT */
    socRxReasonDosAttack,          /* 15: CPU_DOS_ATTACK */
    socRxReasonMartianAddr,        /* 16: CPU_MARTIAN_ADDR */
    socRxReasonTunnelError,        /* 17: CPU_TUNNEL_ERR */
    socRxReasonInvalid,            /* 18: RESERVED_0 */ 
    socRxReasonIcmpRedirect,       /* 19: ICMP_REDIRECT */
    socRxReasonL3Slowpath,         /* 20: L3_SLOWPATH */
    socRxReasonParityError,        /* 21: PARITY_ERROR */
    socRxReasonL3MtuFail,          /* 22: L3_MTU_CHECK_FAIL */
    socRxReasonHigigHdrError,      /* 23: HGHDR_ERROR */
    socRxReasonMcastIdxError,      /* 24: MCIDX_ERROR */
    socRxReasonVlanFilterMatch,    /* 25: VFP */
    socRxReasonClassBasedMove,     /* 26: CBSM_PREVENTED */
    socRxReasonL3AddrBindFail,     /* 27: MAC_BIND_FAIL */
    socRxReasonMplsLabelMiss,      /* 28: MPLS_LABEL_MISS */
    socRxReasonMplsInvalidAction,  /* 29: MPLS_INVALID_ACTION */
    socRxReasonMplsInvalidPayload, /* 30: MPLS_INVALID_PAYLOAD */
    socRxReasonMplsTtl,            /* 31: MPLS_TTL_CHECK_FAIL */
    socRxReasonMplsSequenceNumber, /* 32: MPLS_SEQ_NUM_FAIL */
    socRxReasonL2NonUnicastMiss,   /* 33: PBT_NONUC_PKT */
    socRxReasonNhop,               /* 34: L3_NEXT_HOP */
    socRxReasonMplsUnknownAch,     /* 35: MPLS_UNKNOWN_ACH_ERROR */
    socRxReasonStation,            /* 36: MY_STATION */
    socRxReasonNiv,                /* 37: NIV_DROP_REASON_ENCODING */
    socRxReasonNiv,                /* 38: ->  */
    socRxReasonNiv,                /* 39: 3-bit */
    socRxReasonVlanTranslate,      /* 40: VXLT_MISS */
    socRxReasonTimeSync,           /* 41: TIME_SYNC */
    socRxReasonOAMSlowpath,        /* 42: OAM_SLOWPATH */
    socRxReasonOAMError,           /* 43: OAM_ERROR */
    socRxReasonTrill,              /* 44: TRILL_DROP_REASON_ENCODING */
    socRxReasonTrill,              /* 45: -> */
    socRxReasonTrill,              /* 46: 3-bit */
    socRxReasonL2GreSipMiss,       /* 47: L2GRE_SIP_MISS */
    socRxReasonL2GreVpnIdMiss,     /* 48: L2GRE_VPNID_MISS */
    socRxReasonBfdSlowpath,        /* 49: BFD_SLOWPATH */
    socRxReasonBfd,                /* 50: BFD_ERROR */
    socRxReasonOAMLMDM,            /* 51: OAM_LMDM */
    socRxReasonCongestionCnm,      /* 52: ICNM */
    socRxReasonMplsIllegalReservedLabel, /* 53: MPLS_ILLEGAL_RESERVED_LABEL */
    socRxReasonMplsRouterAlertLabel, /* 54: MPLS_ALERT_LABEL */
    socRxReasonCongestionCnmProxy, /* 55: QCN_CNM_PRP */
    socRxReasonCongestionCnmProxyError, /* 56: QCN_CNM_PRP_DLF */
    socRxReasonVxlanSipMiss,       /* 57: VXLAN_SIP_MISS */
    socRxReasonVxlanVpnIdMiss,     /* 58: VXLAN_VN_ID_MISS */
    socRxReasonFcoeZoneCheckFail,  /* 59: FCOE_ZONE_CHECK_FAIL */
    socRxReasonNat,                /* 60: NAT_DROP_REASON_ENCODING */
    socRxReasonNat,                /* 61: -> */
    socRxReasonNat,                /* 62: 3-bit */
    socRxReasonIpmcInterfaceMismatch /* 63: CPU_IPMC_INTERFACE_MISMATCH */
};
/* From FORMAT TRILL_CPU_OPCODE_ENCODING */
static soc_rx_reason_t dcb26_rx_reason_trill_encoding_map[] = {
    socRxReasonTrill,              /* 0:NO_ERRORS
                                    * Base field, must match the entries above */
    socRxReasonTrillInvalid,       /* 1:TRILL_HDR_ERROR */
    socRxReasonTrillMiss,          /* 2:TRILL_LOOKUP_MISS */
    socRxReasonTrillRpfFail,       /* 3:TRILL_RPF_CHECK_FAIL */
    socRxReasonTrillSlowpath,      /* 4:TRILL_SLOWPATH */
    socRxReasonTrillCoreIsIs,      /* 5:TRILL_CORE_IS_IS_PKT */
    socRxReasonTrillTtl,           /* 6:TRILL_HOP_COUNT_CHECK_FAIL */
    socRxReasonTrillName,          /* 7:NICKNAME_TABLE_COPYTOCPU */
    socRxReasonInvalid,            /* Offset 8 */
    socRxReasonInvalid,            /* Offset 9 */
    socRxReasonInvalid,            /* Offset 10 */
    socRxReasonInvalid,            /* Offset 11 */
    socRxReasonInvalid,            /* Offset 12 */
    socRxReasonInvalid,            /* Offset 13 */
    socRxReasonInvalid,            /* Offset 14 */
    socRxReasonInvalid,            /* Offset 15 */
    socRxReasonInvalid,            /* Offset 16 */
    socRxReasonInvalid,            /* Offset 17 */
    socRxReasonInvalid,            /* Offset 18 */
    socRxReasonInvalid,            /* Offset 19 */
    socRxReasonInvalid,            /* Offset 20 */
    socRxReasonInvalid,            /* Offset 21 */
    socRxReasonInvalid,            /* Offset 22 */
    socRxReasonInvalid,            /* Offset 23 */
    socRxReasonInvalid,            /* Offset 24 */
    socRxReasonInvalid,            /* Offset 25 */
    socRxReasonInvalid,            /* Offset 26 */
    socRxReasonInvalid,            /* Offset 27 */
    socRxReasonInvalid,            /* Offset 28 */
    socRxReasonInvalid,            /* Offset 29 */
    socRxReasonInvalid,            /* Offset 30 */
    socRxReasonInvalid,            /* Offset 31 */
    socRxReasonInvalid,            /* Offset 32 */
    socRxReasonInvalid,            /* Offset 33 */
    socRxReasonInvalid,            /* Offset 34 */
    socRxReasonInvalid,            /* Offset 35 */
    socRxReasonInvalid,            /* Offset 36 */
    socRxReasonInvalid,            /* Offset 37 */
    socRxReasonInvalid,            /* Offset 38 */
    socRxReasonInvalid,            /* Offset 39 */
    socRxReasonInvalid,            /* Offset 40 */
    socRxReasonInvalid,            /* Offset 41 */
    socRxReasonInvalid,            /* Offset 42 */
    socRxReasonInvalid,            /* Offset 43 */
    socRxReasonInvalid,            /* Offset 44 */
    socRxReasonInvalid,            /* Offset 45 */
    socRxReasonInvalid,            /* Offset 46 */
    socRxReasonInvalid,            /* Offset 47 */
    socRxReasonInvalid,            /* Offset 48 */
    socRxReasonInvalid,            /* Offset 49 */
    socRxReasonInvalid,            /* Offset 50 */
    socRxReasonInvalid,            /* Offset 51 */
    socRxReasonInvalid,            /* Offset 52 */
    socRxReasonInvalid,            /* Offset 53 */
    socRxReasonInvalid,            /* Offset 54 */
    socRxReasonInvalid,            /* Offset 55 */
    socRxReasonInvalid,            /* Offset 56 */
    socRxReasonInvalid,            /* Offset 57 */
    socRxReasonInvalid,            /* Offset 58 */
    socRxReasonInvalid,            /* Offset 59 */
    socRxReasonInvalid,            /* Offset 60 */
    socRxReasonInvalid,            /* Offset 61 */
    socRxReasonInvalid,            /* Offset 62 */
    socRxReasonInvalid             /* Offset 63 */
};
/* From FORMAT NAT_CPU_OPCODE_ENCODING */
static soc_rx_reason_t dcb26_rx_reason_nat_encoding_map[] = {
    socRxReasonNat,                /* 0:NOP
                                    * Base field, must match the entries above */
    socRxReasonTcpUdpNatMiss,      /* 1:NORMAL */
    socRxReasonIcmpNatMiss,        /* 2:ICMP */
    socRxReasonNatFragment,        /* 3:FRAGMEMT */
    socRxReasonNatMiss,            /* 4:OTHER */
    socRxReasonInvalid,            /* Offset 5 */
    socRxReasonInvalid,            /* Offset 6 */
    socRxReasonInvalid,            /* Offset 7 */
    socRxReasonInvalid,            /* Offset 8 */
    socRxReasonInvalid,            /* Offset 9 */
    socRxReasonInvalid,            /* Offset 10 */
    socRxReasonInvalid,            /* Offset 11 */
    socRxReasonInvalid,            /* Offset 12 */
    socRxReasonInvalid,            /* Offset 13 */
    socRxReasonInvalid,            /* Offset 14 */
    socRxReasonInvalid,            /* Offset 15 */
    socRxReasonInvalid,            /* Offset 16 */
    socRxReasonInvalid,            /* Offset 17 */
    socRxReasonInvalid,            /* Offset 18 */
    socRxReasonInvalid,            /* Offset 19 */
    socRxReasonInvalid,            /* Offset 20 */
    socRxReasonInvalid,            /* Offset 21 */
    socRxReasonInvalid,            /* Offset 22 */
    socRxReasonInvalid,            /* Offset 23 */
    socRxReasonInvalid,            /* Offset 24 */
    socRxReasonInvalid,            /* Offset 25 */
    socRxReasonInvalid,            /* Offset 26 */
    socRxReasonInvalid,            /* Offset 27 */
    socRxReasonInvalid,            /* Offset 28 */
    socRxReasonInvalid,            /* Offset 29 */
    socRxReasonInvalid,            /* Offset 30 */
    socRxReasonInvalid,            /* Offset 31 */
    socRxReasonInvalid,            /* Offset 32 */
    socRxReasonInvalid,            /* Offset 33 */
    socRxReasonInvalid,            /* Offset 34 */
    socRxReasonInvalid,            /* Offset 35 */
    socRxReasonInvalid,            /* Offset 36 */
    socRxReasonInvalid,            /* Offset 37 */
    socRxReasonInvalid,            /* Offset 38 */
    socRxReasonInvalid,            /* Offset 39 */
    socRxReasonInvalid,            /* Offset 40 */
    socRxReasonInvalid,            /* Offset 41 */
    socRxReasonInvalid,            /* Offset 42 */
    socRxReasonInvalid,            /* Offset 43 */
    socRxReasonInvalid,            /* Offset 44 */
    socRxReasonInvalid,            /* Offset 45 */
    socRxReasonInvalid,            /* Offset 46 */
    socRxReasonInvalid,            /* Offset 47 */
    socRxReasonInvalid,            /* Offset 48 */
    socRxReasonInvalid,            /* Offset 49 */
    socRxReasonInvalid,            /* Offset 50 */
    socRxReasonInvalid,            /* Offset 51 */
    socRxReasonInvalid,            /* Offset 52 */
    socRxReasonInvalid,            /* Offset 53 */
    socRxReasonInvalid,            /* Offset 54 */
    socRxReasonInvalid,            /* Offset 55 */
    socRxReasonInvalid,            /* Offset 56 */
    socRxReasonInvalid,            /* Offset 57 */
    socRxReasonInvalid,            /* Offset 58 */
    socRxReasonInvalid,            /* Offset 59 */
    socRxReasonInvalid,            /* Offset 60 */
    socRxReasonInvalid,            /* Offset 61 */
    socRxReasonInvalid,            /* Offset 62 */
    socRxReasonInvalid             /* Offset 63 */
};
static soc_rx_reason_t *dcb26_rx_reason_maps[] = {
    dcb26_rx_reason_map,
    dcb21_rx_reason_niv_encoding_map,
    dcb26_rx_reason_trill_encoding_map,
    dcb26_rx_reason_nat_encoding_map,
    NULL
};

static _shr_rx_decap_tunnel_t dcb26_rx_decap_tunnel_map[] = {
    _SHR_RX_DECAP_NONE,
    _SHR_RX_DECAP_ACCESS_SVP,
    _SHR_RX_DECAP_MIM,
    _SHR_RX_DECAP_L2GRE,
    _SHR_RX_DECAP_VXLAN,
    _SHR_RX_DECAP_AMT,
    _SHR_RX_DECAP_IP,
    _SHR_RX_DECAP_TRILL,
    _SHR_RX_DECAP_L2MPLS_1LABEL,
    _SHR_RX_DECAP_L2MPLS_2LABEL,
    _SHR_RX_DECAP_L2MPLS_1LABELCW,
    _SHR_RX_DECAP_L2MPLS_2LABELCW,
    _SHR_RX_DECAP_L3MPLS_1LABEL,
    _SHR_RX_DECAP_L3MPLS_2LABEL,
    _SHR_RX_DECAP_L3MPLS_1LABELCW,
    _SHR_RX_DECAP_L3MPLS_2LABELCW
}; 
static uint32 dcb26_rx_decap_tunnel_get(dcb_t *dcb) {
    dcb26_t *d = (dcb26_t *)dcb;
    /* Check for array bounds */
    if ( (d->word11.overlay1.decap_tunnel_type <  
    (sizeof (dcb26_rx_decap_tunnel_map) / sizeof (_shr_rx_decap_tunnel_t))) ) {
        return (uint32)dcb26_rx_decap_tunnel_map[d->word11.overlay1.decap_tunnel_type];
    }
    return (uint32)_SHR_RX_DECAP_NONE;
}
static uint32
dcb26_rx_untagged_get(dcb_t *dcb, int dt_mode, int ingport_is_hg)
{
    dcb26_t *d = (dcb26_t *)dcb;

    COMPILER_REFERENCE(dt_mode);

    return (ingport_is_hg ?
            ((d->tag_status) ? 0 : 2) :
            ((d->tag_status & 0x2) ?
             ((d->tag_status & 0x1) ? 0 : 2) :
             ((d->tag_status & 0x1) ? 1 : 3)));
}

static uint32 
dcb26_rx_reason_get(dcb_t *dcb)
{
    dcb26_t *d = (dcb26_t *)dcb;

    return (d->switch_pkt) ? (d->reason) : 0;
}

static uint32 
dcb26_rx_reason_hi_get(dcb_t *dcb)
{
    dcb26_t *d = (dcb26_t *)dcb;

    return (d->switch_pkt) ? (d->reason_hi) : 0;
}

GETFUNCFIELD(26, xfercount, count)
GETFUNCFIELD(26, rx_cos, word4.overlay2.cpu_cos)

/* Fields extracted from MH/PBI */
GETHG2FUNCFIELD(26, rx_destmod, dst_mod)
GETHG2FUNCFIELD(26, rx_destport, dst_port)
GETHG2FUNCFIELD(26, rx_srcmod, src_mod)
GETHG2FUNCFIELD(26, rx_srcport, src_port)
GETHG2FUNCFIELD(26, rx_opcode, opcode)
GETHG2FUNCFIELD(26, rx_prio, vlan_pri) /* outer_pri */
GETHG2FUNCEXPR(26, rx_mcast, ((h->ppd_overlay1.dst_mod << 8) |
                              (h->ppd_overlay1.dst_port)))
GETHG2FUNCEXPR(26, rx_vclabel, ((h->ppd_overlay1.vc_label_19_16 << 16) |
                              (h->ppd_overlay1.vc_label_15_8 << 8) |
                              (h->ppd_overlay1.vc_label_7_0)))
GETHG2FUNCEXPR(26, rx_classtag, (h->ppd_overlay1.ppd_type != 1 ? 0 :
                                 (h->ppd_overlay2.ctag_hi << 8) |
                                 (h->ppd_overlay2.ctag_lo)))
GETFUNCFIELD(26, rx_matchrule, match_rule)
GETFUNCFIELD(26, rx_ingport, srcport)
GETFUNCFIELD(26, rx_timestamp, timestamp)
GETFUNCFIELD(26, rx_timestamp_upper, word14.timestamp_hi)
GETPTREXPR(26, mhp, &(d->mh0))
GETFUNCFIELD(26, outer_vid, word4.overlay1.outer_vid)
GETFUNCFIELD(26, outer_pri, word11.overlay1.outer_pri)
GETFUNCFIELD(26, outer_cfi, word11.overlay1.outer_cfi)
GETFUNCFIELD(26, inner_vid, word11.overlay1.inner_vid)
GETFUNCFIELD(26, inner_pri, inner_pri)
GETFUNCFIELD(26, inner_cfi, word11.overlay1.inner_cfi)
GETFUNCFIELD(26, rx_bpdu, bpdu)
GETFUNCEXPR(26, rx_l3_intf,
            (((d->repl_nhi) & 0xffff) + _SHR_L3_EGRESS_IDX_MIN))

dcb_op_t dcb26_op = {
    26,
    sizeof(dcb26_t),
    dcb26_rx_reason_maps,
    dcb0_rx_reason_map_get,
    dcb21_rx_reasons_get,
    dcb19_init,
    dcb19_addtx,
    dcb19_addrx,
    dcb19_intrinfo,
    dcb19_reqcount_set,
    dcb19_reqcount_get,
    dcb26_xfercount_get,
    dcb0_addr_set,
    dcb0_addr_get,
    dcb0_paddr_get,
    dcb19_done_set,
    dcb19_done_get,
    dcb19_sg_set,
    dcb19_sg_get,
    dcb19_chain_set,
    dcb19_chain_get,
    dcb19_reload_set,
    dcb19_reload_get,
    dcb19_desc_intr_set,
    dcb19_desc_intr_get,
    dcb19_tx_l2pbm_set,
    dcb19_tx_utpbm_set,
    dcb19_tx_l3pbm_set,
    dcb19_tx_crc_set,
    dcb19_tx_cos_set,
    dcb19_tx_destmod_set,
    dcb19_tx_destport_set,
    dcb19_tx_opcode_set,
    dcb19_tx_srcmod_set,
    dcb19_tx_srcport_set,
    dcb19_tx_prio_set,
    dcb19_tx_pfm_set,
    dcb26_rx_untagged_get,
    dcb19_rx_crc_get,
    dcb26_rx_cos_get,
    dcb26_rx_destmod_get,
    dcb26_rx_destport_get,
    dcb26_rx_opcode_get,
    dcb26_rx_classtag_get,
    dcb26_rx_matchrule_get,
    dcb19_rx_start_get,
    dcb19_rx_end_get,
    dcb19_rx_error_get,
    dcb26_rx_prio_get,
    dcb26_rx_reason_get,
    dcb26_rx_reason_hi_get,
    dcb26_rx_ingport_get,
    dcb26_rx_srcport_get,
    dcb26_rx_srcmod_get,
    dcb26_rx_mcast_get,
    dcb26_rx_vclabel_get,
    dcb23_rx_mirror_get,
    dcb26_rx_timestamp_get,
    dcb26_rx_timestamp_upper_get,
    dcb19_hg_set,
    dcb19_hg_get,
    dcb19_stat_set,
    dcb19_stat_get,
    dcb19_purge_set,
    dcb19_purge_get,
    dcb26_mhp_get,
    dcb26_outer_vid_get,
    dcb26_outer_pri_get,
    dcb26_outer_cfi_get,
    dcb23_rx_outer_tag_action_get,
    dcb26_inner_vid_get,
    dcb26_inner_pri_get,
    dcb26_inner_cfi_get,
    dcb23_rx_inner_tag_action_get,
    dcb26_rx_bpdu_get,
    dcb26_rx_l3_intf_get,
    dcb26_rx_decap_tunnel_get,
    dcb23_rx_switch_drop_get,
    dcb23_olp_encap_oam_pkt_get,
    dcb23_read_ecc_error_get,
    dcb23_desc_remaining_get,
    dcb23_desc_remaining_set,
    dcb23_desc_status_get,
    dcb23_desc_status_set,
    NULL,
};
#endif /* BCM_TRIDENT2_SUPPORT */

#if defined(BCM_KATANA2_SUPPORT)
/*
 * DCB Type 29 Support
 */
static soc_rx_reason_t
dcb29_rx_reason_map_ip_0[] = { /* IP Overlay 0 */
    socRxReasonUnknownVlan,        /* Offset 0 */
    socRxReasonL2SourceMiss,       /* Offset 1 */
    socRxReasonL2DestMiss,         /* Offset 2 */
    socRxReasonL2Move,             /* Offset 3 */
    socRxReasonL2Cpu,              /* Offset 4 */
    socRxReasonSampleSource,       /* Offset 5 */
    socRxReasonSampleDest,         /* Offset 6 */
    socRxReasonL3SourceMiss,       /* Offset 7 */
    socRxReasonL3DestMiss,         /* Offset 8 */
    socRxReasonL3SourceMove,       /* Offset 9 */
    socRxReasonMcastMiss,          /* Offset 10 */
    socRxReasonIpMcastMiss,        /* Offset 11 */
    socRxReasonFilterMatch,        /* Offset 12 */
    socRxReasonL3HeaderError,      /* Offset 13 */
    socRxReasonProtocol,           /* Offset 14 */
    socRxReasonDosAttack,          /* Offset 15 */
    socRxReasonMartianAddr,        /* Offset 16 */
    socRxReasonTunnelError,        /* Offset 17 */
    socRxReasonMirror,             /* Offset 18 */ 
    socRxReasonIcmpRedirect,       /* Offset 19 */
    socRxReasonL3Slowpath,         /* Offset 20 */
    socRxReasonParityError,        /* Offset 21 */
    socRxReasonL3MtuFail,          /* Offset 22 */
    socRxReasonHigigHdrError,      /* Offset 23 */
    socRxReasonMcastIdxError,      /* Offset 24 */
    socRxReasonVlanFilterMatch,    /* Offset 25 */
    socRxReasonClassBasedMove,     /* Offset 26 */
    socRxReasonL3AddrBindFail,     /* Offset 27 */
    socRxReasonMplsLabelMiss,      /* Offset 28 */
    socRxReasonMplsInvalidAction,  /* Offset 29 */
    socRxReasonMplsInvalidPayload, /* Offset 30 */
    socRxReasonMplsTtl,            /* Offset 31 */
    socRxReasonMplsSequenceNumber, /* Offset 32 */
    socRxReasonL2NonUnicastMiss,   /* Offset 33 */
    socRxReasonNhop,               /* Offset 34 */
    socRxReasonBfdSlowpath,        /* Offset 35 */
    socRxReasonStation,            /* Offset 36 */
    socRxReasonVlanTranslate,      /* Offset 37 */
    socRxReasonTimeSync,           /* Offset 38 */
    socRxReasonL2LearnLimit,       /* Offset 39 */
    socRxReasonBfd,                /* Offset 40 */
    socRxReasonFailoverDrop,       /* Offset 41 */
    socRxReasonUnknownSubtendingPort,      /* Offset 42 */
    socRxReasonMplsReservedEntropyLabel,   /* Offset 43 */
    socRxReasonLLTagAbsentDrop,            /* Offset 44 */
    socRxReasonLLTagPresentDrop,           /* Offset 45 */
    socRxReasonMplsLookupsExceeded,        /* Offset 46 */
    socRxReasonMplsIllegalReservedLabel,   /* Offset 47 */
    socRxReasonMplsRouterAlertLabel,       /* Offset 48 */
    socRxReasonMplsUnknownAch,             /* Offset 49 */
    socRxReasonInvalid,                    /* Offset 50 */
    socRxReasonInvalid,            /* Offset 51 */
    socRxReasonInvalid,            /* Offset 52 */
    socRxReasonInvalid,            /* Offset 53 */
    socRxReasonInvalid,            /* Offset 54 */
    socRxReasonInvalid,            /* Offset 55 */
    socRxReasonInvalid,            /* Offset 56 */
    socRxReasonInvalid,            /* Offset 57 */
    socRxReasonInvalid,            /* Offset 58 */
    socRxReasonInvalid,            /* Offset 59 */
    socRxReasonInvalid,            /* Offset 60 */
    socRxReasonInvalid,            /* Offset 61 */
    socRxReasonInvalid,            /* Offset 62 */
    socRxReasonInvalid             /* Offset 63 */
};

static soc_rx_reason_t
dcb29_rx_reason_map_ip_1[] = { /* IP Overlay 1 */
    socRxReasonUnknownVlan,        /* Offset 0 */
    socRxReasonL2SourceMiss,       /* Offset 1 */
    socRxReasonL2DestMiss,         /* Offset 2 */
    socRxReasonL2Move,             /* Offset 3 */
    socRxReasonL2Cpu,              /* Offset 4 */
    socRxReasonSampleSource,       /* Offset 5 */
    socRxReasonSampleDest,         /* Offset 6 */
    socRxReasonL3SourceMiss,       /* Offset 7 */
    socRxReasonL3DestMiss,         /* Offset 8 */
    socRxReasonL3SourceMove,       /* Offset 9 */
    socRxReasonMcastMiss,          /* Offset 10 */
    socRxReasonIpMcastMiss,        /* Offset 11 */
    socRxReasonFilterMatch,        /* Offset 12 */
    socRxReasonL3HeaderError,      /* Offset 13 */
    socRxReasonProtocol,           /* Offset 14 */
    socRxReasonDosAttack,          /* Offset 15 */
    socRxReasonMartianAddr,        /* Offset 16 */
    socRxReasonTunnelError,        /* Offset 17 */
    socRxReasonMirror,             /* Offset 18 */ 
    socRxReasonIcmpRedirect,       /* Offset 19 */
    socRxReasonL3Slowpath,         /* Offset 20 */
    socRxReasonParityError,        /* Offset 21 */
    socRxReasonL3MtuFail,          /* Offset 22 */
    socRxReasonHigigHdrError,      /* Offset 23 */
    socRxReasonMcastIdxError,      /* Offset 24 */
    socRxReasonVlanFilterMatch,    /* Offset 25 */
    socRxReasonClassBasedMove,     /* Offset 26 */
    socRxReasonL3AddrBindFail,     /* Offset 27 */
    socRxReasonMplsLabelMiss,      /* Offset 28 */
    socRxReasonMplsInvalidAction,  /* Offset 29 */
    socRxReasonMplsInvalidPayload, /* Offset 30 */
    socRxReasonMplsTtl,            /* Offset 31 */
    socRxReasonMplsSequenceNumber, /* Offset 32 */
    socRxReasonL2NonUnicastMiss,   /* Offset 33 */
    socRxReasonNhop,               /* Offset 34 */
    socRxReasonBfdSlowpath,        /* Offset 35 */
    socRxReasonStation,            /* Offset 36 */
    socRxReasonVlanTranslate,      /* Offset 37 */
    socRxReasonTimeSync,           /* Offset 38 */
    socRxReasonL2LearnLimit,       /* Offset 39 */
    socRxReasonBfd,                /* Offset 40 */
    socRxReasonFailoverDrop,       /* Offset 41 */
    socRxReasonOAMSlowpath,        /* Offset 42 */
    socRxReasonOAMError,           /* Offset 43 */
    socRxReasonOAMLMDM,            /* Offset 44 */
    socRxReasonOAMCCMSlowPath,     /* Offset 45 */
    socRxReasonOAMIncompleteOpcode,/* Offset 46 */
    socRxReasonInvalid,            /* Offset 47 */
    socRxReasonSat,                /* Offset 48 */
    socRxReasonOAMMplsLmDm,        /* Offset 49 */
    socRxReasonInvalid,            /* Offset 50 */
    socRxReasonInvalid,            /* Offset 51 */
    socRxReasonInvalid,            /* Offset 52 */
    socRxReasonInvalid,            /* Offset 53 */
    socRxReasonInvalid,            /* Offset 54 */
    socRxReasonInvalid,            /* Offset 55 */
    socRxReasonInvalid,            /* Offset 56 */
    socRxReasonInvalid,            /* Offset 57 */
    socRxReasonInvalid,            /* Offset 58 */
    socRxReasonInvalid,            /* Offset 59 */
    socRxReasonInvalid,            /* Offset 60 */
    socRxReasonInvalid,            /* Offset 61 */
    socRxReasonInvalid,            /* Offset 62 */
    socRxReasonInvalid             /* Offset 63 */
};
static soc_rx_reason_t
dcb29_rx_reason_map_ip_3[] = { /* IP Overlay 3 */
    socRxReasonUnknownVlan,        /* Offset 0 */
    socRxReasonL2SourceMiss,       /* Offset 1 */
    socRxReasonL2DestMiss,         /* Offset 2 */
    socRxReasonL2Move,             /* Offset 3 */
    socRxReasonL2Cpu,              /* Offset 4 */
    socRxReasonSampleSource,       /* Offset 5 */
    socRxReasonSampleDest,         /* Offset 6 */
    socRxReasonL3SourceMiss,       /* Offset 7 */
    socRxReasonL3DestMiss,         /* Offset 8 */
    socRxReasonL3SourceMove,       /* Offset 9 */
    socRxReasonMcastMiss,          /* Offset 10 */
    socRxReasonIpMcastMiss,        /* Offset 11 */
    socRxReasonFilterMatch,        /* Offset 12 */
    socRxReasonL3HeaderError,      /* Offset 13 */
    socRxReasonProtocol,           /* Offset 14 */
    socRxReasonDosAttack,          /* Offset 15 */
    socRxReasonMartianAddr,        /* Offset 16 */
    socRxReasonTunnelError,        /* Offset 17 */
    socRxReasonMirror,             /* Offset 18 */ 
    socRxReasonIcmpRedirect,       /* Offset 19 */
    socRxReasonL3Slowpath,         /* Offset 20 */
    socRxReasonParityError,        /* Offset 21 */
    socRxReasonL3MtuFail,          /* Offset 22 */
    socRxReasonHigigHdrError,      /* Offset 23 */
    socRxReasonMcastIdxError,      /* Offset 24 */
    socRxReasonVlanFilterMatch,    /* Offset 25 */
    socRxReasonClassBasedMove,     /* Offset 26 */
    socRxReasonL3AddrBindFail,     /* Offset 27 */
    socRxReasonMplsLabelMiss,      /* Offset 28 */
    socRxReasonMplsInvalidAction,  /* Offset 29 */
    socRxReasonMplsInvalidPayload, /* Offset 30 */
    socRxReasonMplsTtl,            /* Offset 31 */
    socRxReasonMplsSequenceNumber, /* Offset 32 */
    socRxReasonL2NonUnicastMiss,   /* Offset 33 */
    socRxReasonNhop,               /* Offset 34 */
    socRxReasonBfdSlowpath,        /* Offset 35 */
    socRxReasonStation,            /* Offset 36 */
    socRxReasonVlanTranslate,      /* Offset 37 */
    socRxReasonTimeSync,           /* Offset 38 */
    socRxReasonL2LearnLimit,       /* Offset 39 */
    socRxReasonBfd,                /* Offset 40 */
    socRxReasonFailoverDrop,       /* Offset 41 */
    socRxReasonNivUntagDrop,       /* Offset 42 */
    socRxReasonNivTagDrop,         /* Offset 43 */
    socRxReasonNivTagInvalid,      /* Offset 44 */
    socRxReasonNivRpfFail,         /* Offset 45 */
    socRxReasonNivInterfaceMiss,   /* Offset 46 */
    socRxReasonNivPrioDrop,        /* Offset 47 */
    socRxReasonInvalid,            /* Offset 48 */
    socRxReasonInvalid,            /* Offset 49 */
    socRxReasonInvalid,            /* Offset 50 */
    socRxReasonInvalid,            /* Offset 51 */
    socRxReasonInvalid,            /* Offset 52 */
    socRxReasonInvalid,            /* Offset 53 */
    socRxReasonInvalid,            /* Offset 54 */
    socRxReasonInvalid,            /* Offset 55 */
    socRxReasonInvalid,            /* Offset 56 */
    socRxReasonInvalid,            /* Offset 57 */
    socRxReasonInvalid,            /* Offset 58 */
    socRxReasonInvalid,            /* Offset 59 */
    socRxReasonInvalid,            /* Offset 60 */
    socRxReasonInvalid,            /* Offset 61 */
    socRxReasonInvalid,            /* Offset 62 */
    socRxReasonInvalid             /* Offset 63 */
  };
static soc_rx_reason_t
dcb29_rx_reason_map_ep[] = {
    socRxReasonUnknownVlan,        /* Offset 0 */
    socRxReasonStp,                /* Offset 1 */
    socRxReasonVlanTranslate,      /* Offset 2 new */
    socRxReasonTunnelError,        /* Offset 3 */
    socRxReasonIpmc,               /* Offset 4 */
    socRxReasonL3HeaderError,      /* Offset 5 */
    socRxReasonTtl,                /* Offset 6 */
    socRxReasonL2MtuFail,          /* Offset 7 */
    socRxReasonHigigHdrError,      /* Offset 8 */
    socRxReasonSplitHorizon,       /* Offset 9 */
    socRxReasonNivPrune,           /* Offset 10 */
    socRxReasonVirtualPortPrune,   /* Offset 11 */
    socRxReasonFilterMatch,        /* Offset 12 */
    socRxReasonNonUnicastDrop,     /* Offset 13 */
    socRxReasonTrillPacketPortMismatch, /* Offset 14 */
    socRxReasonOAMError,           /* Offset 15 */
    socRxReasonOAMLMDM,            /* Offset 16 */
    socRxReasonOAMCCMSlowPath,     /* Offset 17 */
    socRxReasonOAMSlowpath,        /* Offset 18 */
    socRxReasonInvalid,            /* Offset 19 */
    socRxReasonInvalid,            /* Offset 20 */
    socRxReasonInvalid,            /* Offset 21 */
    socRxReasonInvalid,            /* Offset 22 */
    socRxReasonInvalid,            /* Offset 23 */
    socRxReasonInvalid,            /* Offset 24 */
    socRxReasonInvalid,            /* Offset 25 */
    socRxReasonInvalid,            /* Offset 26 */
    socRxReasonInvalid,            /* Offset 27 */
    socRxReasonInvalid,            /* Offset 28 */
    socRxReasonInvalid,            /* Offset 29 */
    socRxReasonInvalid,            /* Offset 30 */
    socRxReasonInvalid,            /* Offset 31 */
    socRxReasonInvalid,            /* Offset 32 */
    socRxReasonInvalid,            /* Offset 33 */
    socRxReasonInvalid,            /* Offset 34 */
    socRxReasonInvalid,            /* Offset 35 */
    socRxReasonInvalid,            /* Offset 36 */
    socRxReasonInvalid,            /* Offset 37 */
    socRxReasonInvalid,            /* Offset 38 */
    socRxReasonInvalid,            /* Offset 39 */
    socRxReasonInvalid,            /* Offset 40 */
    socRxReasonInvalid,            /* Offset 41 */
    socRxReasonInvalid,            /* Offset 42 */
    socRxReasonInvalid,            /* Offset 43 */
    socRxReasonInvalid,            /* Offset 44 */
    socRxReasonInvalid,            /* Offset 45 */
    socRxReasonInvalid,            /* Offset 46 */
    socRxReasonInvalid,            /* Offset 47 */
    socRxReasonInvalid,            /* Offset 48 */
    socRxReasonInvalid,            /* Offset 49 */
    socRxReasonInvalid,            /* Offset 50 */
    socRxReasonInvalid,            /* Offset 51 */
    socRxReasonInvalid,            /* Offset 52 */
    socRxReasonInvalid,            /* Offset 53 */
    socRxReasonInvalid,            /* Offset 54 */
    socRxReasonInvalid,            /* Offset 55 */
    socRxReasonInvalid,            /* Offset 56 */
    socRxReasonInvalid,            /* Offset 57 */
    socRxReasonInvalid,            /* Offset 58 */
    socRxReasonInvalid,            /* Offset 59 */
    socRxReasonInvalid,            /* Offset 60 */
    socRxReasonInvalid,            /* Offset 61 */
    socRxReasonInvalid,            /* Offset 62 */
    socRxReasonInvalid             /* Offset 63 */
};


static soc_rx_reason_t *dcb29_rx_reason_maps[] = {
    dcb29_rx_reason_map_ip_0,
    dcb29_rx_reason_map_ip_1,
    dcb29_rx_reason_map_ip_3,
    dcb29_rx_reason_map_ep,
    NULL
};
/*
 * Function:
 *      dcb29_rx_reason_map_get
 * Purpose:
 *      Return the RX reason map for DCB 23 type.
 * Parameters:
 *      dcb_op - DCB operations
 *      dcb    - dma control block
 * Returns:
 *      RX reason map pointer
 */
static soc_rx_reason_t *
dcb29_rx_reason_map_get(dcb_op_t *dcb_op, dcb_t *dcb)
{
    soc_rx_reason_t *map = NULL;
    dcb29_t  *d = (dcb29_t *)dcb;

    switch (d->word4.overlay1.cpu_opcode_type) {
    case SOC_CPU_OPCODE_TYPE_IP_0:
        map = dcb29_rx_reason_map_ip_0;
        break;
    case SOC_CPU_OPCODE_TYPE_IP_1:
        map = dcb29_rx_reason_map_ip_1;
        break;
    case SOC_CPU_OPCODE_TYPE_IP_3:
        map = dcb29_rx_reason_map_ip_3;
        break;
    case SOC_CPU_OPCODE_TYPE_EP:
        map = dcb29_rx_reason_map_ep;
        break;
    default:
        /* Unknown reason type */
        break;
    }

    return map;
}

static _shr_rx_decap_tunnel_t dcb29_rx_decap_tunnel_map[] = {
    _SHR_RX_DECAP_NONE,
    _SHR_RX_DECAP_ACCESS_SVP,
    _SHR_RX_DECAP_MIM,
    _SHR_RX_DECAP_NONE,
    _SHR_RX_DECAP_NONE,
    _SHR_RX_DECAP_NONE,
    _SHR_RX_DECAP_IP,
    _SHR_RX_DECAP_NONE,
    _SHR_RX_DECAP_L2MPLS_1LABEL,
    _SHR_RX_DECAP_L2MPLS_1LABELCW,
    _SHR_RX_DECAP_L3MPLS_1LABEL,
    _SHR_RX_DECAP_L3MPLS_1LABELCW,
}; 
static uint32 dcb29_rx_decap_tunnel_get(dcb_t *dcb) {
    dcb29_t *d = (dcb29_t *)dcb;
    uint32 decap_tunnel_type = d->word11.overlay1.decap_tunnel_type;
    /* Check for array bounds */
    if ( (decap_tunnel_type <  
    (sizeof (dcb29_rx_decap_tunnel_map) / sizeof (_shr_rx_decap_tunnel_t))) ) {
        return (uint32)dcb29_rx_decap_tunnel_map[decap_tunnel_type];
    }
    return (uint32)_SHR_RX_DECAP_NONE;
}

GETFUNCEXPR(29, rx_l3_intf, ((d->replicated) ? (d->repl_nhi) :
                             (((d->repl_nhi) & 0x3fff) +
                              _SHR_L3_EGRESS_IDX_MIN)))

GETFUNCFIELD(29, outer_vid, word4.overlay1.outer_vid)
GETFUNCFIELD(29, olp_encap_oam_pkt, word4.overlay3.olp_encap_oam_pkt)
GETFUNCNULL(29, read_ecc_error)
GETFUNCNULL(29, desc_remaining)
SETFUNCNULL(29, desc_remaining)
GETFUNCNULL(29, desc_status)
SETFUNCNULL(29, desc_status)

dcb_op_t dcb29_op = {
    29,
    sizeof(dcb29_t),
    dcb29_rx_reason_maps,
    dcb29_rx_reason_map_get,
    dcb0_rx_reasons_get,
    dcb19_init,
    dcb19_addtx,
    dcb19_addrx,
    dcb19_intrinfo,
    dcb19_reqcount_set,
    dcb19_reqcount_get,
    dcb23_xfercount_get,
    dcb0_addr_set,
    dcb0_addr_get,
    dcb0_paddr_get,
    dcb19_done_set,
    dcb19_done_get,
    dcb19_sg_set,
    dcb19_sg_get,
    dcb19_chain_set,
    dcb19_chain_get,
    dcb19_reload_set,
    dcb19_reload_get,
    dcb19_desc_intr_set,
    dcb19_desc_intr_get,
    dcb19_tx_l2pbm_set,
    dcb19_tx_utpbm_set,
    dcb19_tx_l3pbm_set,
    dcb19_tx_crc_set,
    dcb19_tx_cos_set,
    dcb19_tx_destmod_set,
    dcb19_tx_destport_set,
    dcb19_tx_opcode_set,
    dcb19_tx_srcmod_set,
    dcb19_tx_srcport_set,
    dcb19_tx_prio_set,
    dcb19_tx_pfm_set,
    dcb23_rx_untagged_get,
    dcb19_rx_crc_get,
    dcb23_rx_cos_get,
    dcb23_rx_destmod_get,
    dcb23_rx_destport_get,
    dcb23_rx_opcode_get,
    dcb23_rx_classtag_get,
    dcb23_rx_matchrule_get,
    dcb19_rx_start_get,
    dcb19_rx_end_get,
    dcb19_rx_error_get,
    dcb23_rx_prio_get,
    dcb23_rx_reason_get,
    dcb23_rx_reason_hi_get,
    dcb23_rx_ingport_get,
    dcb23_rx_srcport_get,
    dcb23_rx_srcmod_get,
    dcb23_rx_mcast_get,
    dcb23_rx_vclabel_get,
    dcb23_rx_mirror_get,
    dcb23_rx_timestamp_get,
    dcb23_rx_timestamp_upper_get,
    dcb19_hg_set,
    dcb19_hg_get,
    dcb19_stat_set,
    dcb19_stat_get,
    dcb19_purge_set,
    dcb19_purge_get,
    dcb23_mhp_get,
    dcb29_outer_vid_get,
    dcb23_outer_pri_get,
    dcb23_outer_cfi_get,
    dcb23_rx_outer_tag_action_get,
    dcb23_inner_vid_get,
    dcb23_inner_pri_get,
    dcb23_inner_cfi_get,
    dcb23_rx_inner_tag_action_get,
    dcb23_rx_bpdu_get,
    dcb29_rx_l3_intf_get,
    dcb29_rx_decap_tunnel_get,
    dcb23_rx_switch_drop_get,
    dcb29_olp_encap_oam_pkt_get,
    dcb29_read_ecc_error_get,
    dcb29_desc_remaining_get,
    dcb29_desc_remaining_set,
    dcb29_desc_status_get,
    dcb29_desc_status_set,
    NULL,
};
#endif /* BCM_KATANA2_SUPPORT */

#if defined(BCM_HURRICANE2_SUPPORT)  
GETFUNCNULL(30, rx_switch_drop)
GETFUNCNULL(30, olp_encap_oam_pkt)
GETFUNCNULL(30, read_ecc_error)
GETFUNCNULL(30, desc_remaining)
SETFUNCNULL(30, desc_remaining)
GETFUNCNULL(30, desc_status)
SETFUNCNULL(30, desc_status)

dcb_op_t dcb30_op = {
    30,
    sizeof(dcb20_t),
    dcb20_rx_reason_maps,
    dcb0_rx_reason_map_get,
    dcb0_rx_reasons_get,
    dcb19_init,
    dcb19_addtx,
    dcb19_addrx,
    dcb19_intrinfo,
    dcb19_reqcount_set,
    dcb19_reqcount_get,
    dcb19_xfercount_get,
    dcb0_addr_set,
    dcb0_addr_get,
    dcb0_paddr_get,
    dcb19_done_set,
    dcb19_done_get,
    dcb19_sg_set,
    dcb19_sg_get,
    dcb19_chain_set,
    dcb19_chain_get,
    dcb19_reload_set,
    dcb19_reload_get,
    dcb19_desc_intr_set,
    dcb19_desc_intr_get,
    dcb19_tx_l2pbm_set,
    dcb19_tx_utpbm_set,
    dcb19_tx_l3pbm_set,
    dcb19_tx_crc_set,
    dcb19_tx_cos_set,
    dcb19_tx_destmod_set,
    dcb19_tx_destport_set,
    dcb19_tx_opcode_set,
    dcb19_tx_srcmod_set,
    dcb19_tx_srcport_set,
    dcb19_tx_prio_set,
    dcb19_tx_pfm_set,
    dcb19_rx_untagged_get,
    dcb19_rx_crc_get,
    dcb19_rx_cos_get,
    dcb19_rx_destmod_get,
    dcb19_rx_destport_get,
    dcb19_rx_opcode_get,
    dcb19_rx_classtag_get,
    dcb20_rx_matchrule_get,
    dcb19_rx_start_get,
    dcb19_rx_end_get,
    dcb19_rx_error_get,
    dcb19_rx_prio_get,
    dcb19_rx_reason_get,
    dcb19_rx_reason_hi_get,
    dcb19_rx_ingport_get,
    dcb19_rx_srcport_get,
    dcb19_rx_srcmod_get,
    dcb19_rx_mcast_get,
    dcb19_rx_vclabel_get,
    dcb19_rx_mirror_get,
    dcb20_rx_timestamp_get,
    dcb20_rx_timestamp_upper_get,
    dcb19_hg_set,
    dcb19_hg_get,
    dcb19_stat_set,
    dcb19_stat_get,
    dcb19_purge_set,
    dcb19_purge_get,
    dcb19_mhp_get,
    dcb19_outer_vid_get,
    dcb19_outer_pri_get,
    dcb19_outer_cfi_get,
    dcb19_rx_outer_tag_action_get,
    dcb19_inner_vid_get,
    dcb19_inner_pri_get,
    dcb19_inner_cfi_get,
    dcb19_rx_inner_tag_action_get,
    dcb19_rx_bpdu_get,
    dcb9_rx_l3_intf_get,
    dcb19_rx_decap_tunnel_get,
    dcb30_rx_switch_drop_get,
    dcb30_olp_encap_oam_pkt_get,
    dcb30_read_ecc_error_get,
    dcb30_desc_remaining_get,
    dcb30_desc_remaining_set,
    dcb30_desc_status_get,
    dcb30_desc_status_set,
    NULL,
};
#endif /* BCM_HURRICANE2_SUPPORT */

#if defined(BCM_GREYHOUND_SUPPORT)  
/*
 * DCB Type 31 Support
 */
static soc_rx_reason_t
dcb31_rx_reason_map[] = {
    socRxReasonUnknownVlan,        /* Offset 0 */
    socRxReasonL2SourceMiss,       /* Offset 1 */
    socRxReasonL2DestMiss,         /* Offset 2 */
    socRxReasonL2Move,             /* Offset 3 */
    socRxReasonL2Cpu,              /* Offset 4 */
    socRxReasonSampleSource,       /* Offset 5 */
    socRxReasonSampleDest,         /* Offset 6 */
    socRxReasonL3SourceMiss,       /* Offset 7 */
    socRxReasonL3DestMiss,         /* Offset 8 */
    socRxReasonL3SourceMove,       /* Offset 9 */
    socRxReasonMcastMiss,          /* Offset 10 */
    socRxReasonIpMcastMiss,        /* Offset 11 */
    socRxReasonFilterMatch,        /* Offset 12 */
    socRxReasonL3HeaderError,      /* Offset 13 */
    socRxReasonProtocol,           /* Offset 14 */
    socRxReasonDosAttack,          /* Offset 15 */
    socRxReasonMartianAddr,        /* Offset 16 */
    socRxReasonTunnelError,        /* Offset 17 */
    socRxReasonL2MtuFail,          /* Offset 18 */
    socRxReasonIcmpRedirect,       /* Offset 19 */
    socRxReasonL3Slowpath,         /* Offset 20 */
    socRxReasonParityError,        /* Offset 21 */
    socRxReasonL3MtuFail,          /* Offset 22 */
    socRxReasonHigigHdrError,      /* Offset 23 */
    socRxReasonMcastIdxError,      /* Offset 24 */
    socRxReasonVlanFilterMatch,    /* Offset 25 */
    socRxReasonClassBasedMove,     /* Offset 26 */
    socRxReasonL2LearnLimit,       /* Offset 27 */
    socRxReasonMplsLabelMiss,      /* Offset 28 */
    socRxReasonMplsInvalidAction,  /* Offset 29 */
    socRxReasonMplsInvalidPayload, /* Offset 30 */
    socRxReasonMplsTtl,            /* Offset 31 */
    socRxReasonMplsSequenceNumber, /* Offset 32 */
    socRxReasonL2NonUnicastMiss,   /* Offset 33 */
    socRxReasonNhop,               /* Offset 34 */
    socRxReasonMplsCtrlWordError,  /* Offset 35 */  
    socRxReasonTimeSync,           /* Offset 36 */ 
    socRxReasonOAMSlowpath,        /* Offset 37 */  
    socRxReasonOAMError,           /* Offset 38 */  
    socRxReasonOAMLMDM,            /* Offset 39 */ 
    socRxReasonL3AddrBindFail,     /* Offset 40 */  
    socRxReasonVlanTranslate,      /* Offset 41 */  
    socRxReasonNiv,                /* Offset 42 */ 
    socRxReasonNiv,                /* Offset 43 */ 
    socRxReasonNiv,                /* Offset 44 */ 
    socRxReasonInvalid,            /* Offset 45 */
    socRxReasonInvalid,            /* Offset 46 */
    socRxReasonInvalid,            /* Offset 47 */
    socRxReasonInvalid,            /* Offset 48 */
    socRxReasonInvalid,            /* Offset 49 */
    socRxReasonInvalid,            /* Offset 50 */
    socRxReasonInvalid,            /* Offset 51 */
    socRxReasonInvalid,            /* Offset 52 */
    socRxReasonInvalid,            /* Offset 53 */
    socRxReasonInvalid,            /* Offset 54 */
    socRxReasonInvalid,            /* Offset 55 */
    socRxReasonInvalid,            /* Offset 56 */
    socRxReasonInvalid,            /* Offset 57 */
    socRxReasonInvalid,            /* Offset 58 */
    socRxReasonInvalid,            /* Offset 59 */
    socRxReasonInvalid,            /* Offset 60 */
    socRxReasonInvalid,            /* Offset 61 */
    socRxReasonInvalid,            /* Offset 62 */
    socRxReasonInvalid             /* Offset 63 */
};
static soc_rx_reason_t *dcb31_rx_reason_maps[] = {
    dcb31_rx_reason_map,
    NULL
};

static uint32
dcb31_rx_untagged_get(dcb_t *dcb, int dt_mode, int ingport_is_hg)
{
    dcb31_t *d = (dcb31_t *)dcb;

    COMPILER_REFERENCE(dt_mode);

    return (ingport_is_hg ?
            ((d->itag_status) ? 0 : 2) :
            ((d->itag_status & 0x2) ?
             ((d->itag_status & 0x1) ? 0 : 2) :
             ((d->itag_status & 0x1) ? 1 : 3)));
}

GETFUNCEXPR(31, rx_mirror, ((d->imirror) | (d->emirror)))

GETFUNCFIELD(31, rx_cos, word4.overlay2.cpu_cos)
GETFUNCFIELD(31, outer_vid, word4.overlay1.outer_vid)
GETFUNCFIELD(31, outer_pri, outer_pri)
GETFUNCFIELD(31, outer_cfi, outer_cfi)
GETFUNCFIELD(31, rx_outer_tag_action, otag_action)
GETFUNCFIELD(31, inner_vid, inner_vid)
GETFUNCFIELD(31, inner_pri, inner_pri)
GETFUNCFIELD(31, inner_cfi, inner_cfi)
GETFUNCFIELD(31, rx_inner_tag_action, itag_action)
GETFUNCFIELD(31, rx_bpdu, bpdu)

GETFUNCFIELD(31, rx_timestamp_upper, timestamp_hi)
GETFUNCEXPR(31, rx_l3_intf, ((d->replicated) ? (d->repl_nhi) :
        (((d->repl_nhi) & 0x3ff) + _SHR_L3_EGRESS_IDX_MIN)))
GETFUNCNULL(31, rx_switch_drop)
GETFUNCNULL(31, olp_encap_oam_pkt)
GETFUNCNULL(31, read_ecc_error)
GETFUNCNULL(31, desc_remaining)
SETFUNCNULL(31, desc_remaining)
GETFUNCNULL(31, desc_status)
SETFUNCNULL(31, desc_status)

dcb_op_t dcb31_op = {
    31,
    sizeof(dcb31_t),
    dcb31_rx_reason_maps,
    dcb0_rx_reason_map_get,
    dcb0_rx_reasons_get,
    dcb19_init,
    dcb19_addtx,
    dcb19_addrx,
    dcb19_intrinfo,
    dcb19_reqcount_set,
    dcb19_reqcount_get,
    dcb19_xfercount_get,
    dcb0_addr_set,
    dcb0_addr_get,
    dcb0_paddr_get,
    dcb19_done_set,
    dcb19_done_get,
    dcb19_sg_set,
    dcb19_sg_get,
    dcb19_chain_set,
    dcb19_chain_get,
    dcb19_reload_set,
    dcb19_reload_get,
    dcb19_desc_intr_set,
    dcb19_desc_intr_get,
    dcb19_tx_l2pbm_set,
    dcb19_tx_utpbm_set,
    dcb19_tx_l3pbm_set,
    dcb19_tx_crc_set,
    dcb19_tx_cos_set,
    dcb19_tx_destmod_set,
    dcb19_tx_destport_set,
    dcb19_tx_opcode_set,
    dcb19_tx_srcmod_set,
    dcb19_tx_srcport_set,
    dcb19_tx_prio_set,
    dcb19_tx_pfm_set,
    dcb31_rx_untagged_get,
    dcb19_rx_crc_get,
    dcb31_rx_cos_get,
    dcb23_rx_destmod_get,
    dcb23_rx_destport_get,
    dcb23_rx_opcode_get,
    dcb23_rx_classtag_get,
    dcb23_rx_matchrule_get,
    dcb19_rx_start_get,
    dcb19_rx_end_get,
    dcb19_rx_error_get,
    dcb23_rx_prio_get,
    dcb23_rx_reason_get,
    dcb23_rx_reason_hi_get,
    dcb23_rx_ingport_get,
    dcb23_rx_srcport_get,
    dcb23_rx_srcmod_get,
    dcb23_rx_mcast_get,
    dcb23_rx_vclabel_get,
    dcb31_rx_mirror_get,
    dcb23_rx_timestamp_get,
    dcb31_rx_timestamp_upper_get,
    dcb19_hg_set,
    dcb19_hg_get,
    dcb19_stat_set,
    dcb19_stat_get,
    dcb19_purge_set,
    dcb19_purge_get,
    dcb23_mhp_get,
    dcb31_outer_vid_get,
    dcb31_outer_pri_get,
    dcb31_outer_cfi_get,
    dcb31_rx_outer_tag_action_get,
    dcb31_inner_vid_get,
    dcb31_inner_pri_get,
    dcb31_inner_cfi_get,
    dcb31_rx_inner_tag_action_get,
    dcb31_rx_bpdu_get,
    dcb31_rx_l3_intf_get,
    dcb19_rx_decap_tunnel_get,
    dcb31_rx_switch_drop_get,
    dcb31_olp_encap_oam_pkt_get,
    dcb31_read_ecc_error_get,
    dcb31_desc_remaining_get,
    dcb31_desc_remaining_set,
    dcb31_desc_status_get,
    dcb31_desc_status_set,
    NULL,
};
#endif /* BCM_GREYHOUND_SUPPORT */

#if defined(BCM_TOMAHAWK_SUPPORT) || defined(BCM_TOMAHAWK2_SUPPORT)
static uint32
dcb32_rx_hg2_ext_eh_type_get(dcb_t *dcb)
{
   uint32 eh_type = 0;
   dcb32_t *d = (dcb32_t *)dcb;

   if (d->hg2_ext_hdr) {
      switch (d->word14.overlay1.eh_type) {
         case 0:
              eh_type = d->word14.overlay1.eh_queue_tag;
              break;
         default:
              break;
      }
   } 
   return eh_type;
}

/*
 * DCB Type 32 Support
 */
/* From FORMAT CPU_OPCODES */
static soc_rx_reason_t
dcb32_rx_reason_map[] = {
    socRxReasonUnknownVlan,        /*  0: CPU_UVLAN */
    socRxReasonL2SourceMiss,       /*  1: CPU_SLF */
    socRxReasonL2DestMiss,         /*  2: CPU_DLF */
    socRxReasonL2Move,             /*  3: CPU_L2MOVE */
    socRxReasonL2Cpu,              /*  4: CPU_L2CPU */
    socRxReasonSampleSource,       /*  5: CPU_SFLOW_SRC */
    socRxReasonSampleDest,         /*  6: CPU_SFLOW_DST */
    socRxReasonL3SourceMiss,       /*  7: CPU_L3SRC_MISS */
    socRxReasonL3DestMiss,         /*  8: CPU_L3DST_MISS */
    socRxReasonL3SourceMove,       /*  9: CPU_L3SRC_MOVE */
    socRxReasonMcastMiss,          /* 10: CPU_MC_MISS */
    socRxReasonIpMcastMiss,        /* 11: CPU_IPMC_MISS */
    socRxReasonFilterMatch,        /* 12: CPU_FFP */
    socRxReasonL3HeaderError,      /* 13: CPU_L3HDR_ERR */
    socRxReasonProtocol,           /* 14: CPU_PROTOCOL_PKT */
    socRxReasonDosAttack,          /* 15: CPU_DOS_ATTACK */
    socRxReasonMartianAddr,        /* 16: CPU_MARTIAN_ADDR */
    socRxReasonTunnelError,        /* 17: CPU_TUNNEL_ERR */
    socRxReasonSampleSourceFlex,   /* 18: CPU_SFLOW_FLEX */
    socRxReasonIcmpRedirect,       /* 19: ICMP_REDIRECT */
    socRxReasonL3Slowpath,         /* 20: L3_SLOWPATH */
    socRxReasonParityError,        /* 21: PARITY_ERROR */
    socRxReasonL3MtuFail,          /* 22: L3_MTU_CHECK_FAIL */
    socRxReasonHigigHdrError,      /* 23: HGHDR_ERROR */
    socRxReasonMcastIdxError,      /* 24: MCIDX_ERROR */
    socRxReasonVlanFilterMatch,    /* 25: VFP */
    socRxReasonClassBasedMove,     /* 26: CBSM_PREVENTED */
    socRxReasonL3AddrBindFail,     /* 27: MAC_BIND_FAIL */
    socRxReasonMplsLabelMiss,      /* 28: MPLS_LABEL_MISS */
    socRxReasonMplsInvalidAction,  /* 29: MPLS_INVALID_ACTION */
    socRxReasonMplsInvalidPayload, /* 30: MPLS_INVALID_PAYLOAD */
    socRxReasonMplsTtl,            /* 31: MPLS_TTL_CHECK_FAIL */
    socRxReasonMplsSequenceNumber, /* 32: MPLS_SEQ_NUM_FAIL */
    socRxReasonL2NonUnicastMiss,   /* 33: PBT_NONUC_PKT */
    socRxReasonNhop,               /* 34: L3_NEXT_HOP */
    socRxReasonMplsUnknownAch,     /* 35: MPLS_UNKNOWN_ACH_ERROR */
    socRxReasonStation,            /* 36: MY_STATION */
    socRxReasonNiv,                /* 37: NIV_DROP_REASON_ENCODING */
    socRxReasonNiv,                /* 38: ->  */
    socRxReasonNiv,                /* 39: 3-bit */
    socRxReasonVlanTranslate,      /* 40: XLT_MISS */
    socRxReasonTimeSync,           /* 41: TIME_SYNC */
    socRxReasonOAMSlowpath,        /* 42: OAM_SLOWPATH */
    socRxReasonOAMError,           /* 43: OAM_ERROR */
    socRxReasonTrill,              /* 44: TRILL_DROP_REASON_ENCODING */
    socRxReasonTrill,              /* 45: -> */
    socRxReasonTrill,              /* 46: 3-bit */
    socRxReasonL2GreSipMiss,       /* 47: L2GRE_SIP_MISS */
    socRxReasonL2GreVpnIdMiss,     /* 48: L2GRE_VPNID_MISS */
    socRxReasonBfdSlowpath,        /* 49: BFD_SLOWPATH */
    socRxReasonBfd,                /* 50: BFD_ERROR */
#if 1
    socRxReasonInvalid,            /* 51: NOT DEFINED */
#endif
    socRxReasonCongestionCnm,      /* 52: ICNM */
    socRxReasonMplsIllegalReservedLabel, /* 53: MPLS_ILLEGAL_RESERVED_LABEL */
    socRxReasonMplsRouterAlertLabel, /* 54: MPLS_ALERT_LABEL */
    socRxReasonCongestionCnmProxy, /* 55: QCN_CNM_PRP */
    socRxReasonCongestionCnmProxyError, /* 56: QCN_CNM_PRP_DLF */
    socRxReasonVxlanSipMiss,       /* 57: VXLAN_SIP_MISS */
    socRxReasonVxlanVpnIdMiss,     /* 58: VXLAN_VN_ID_MISS */
    socRxReasonFcoeZoneCheckFail,  /* 59: FCOE_ZONE_CHECK_FAIL */
    socRxReasonNat,                /* 60: NAT_DROP_REASON_ENCODING */
    socRxReasonNat,                /* 61: -> */
    socRxReasonNat,                /* 62: 3-bit */
    socRxReasonIpmcInterfaceMismatch /* 63: CPU_IPMC_INTERFACE_MISMATCH */
};

static soc_rx_reason_t *dcb32_rx_reason_maps[] = {
    dcb32_rx_reason_map,
    dcb21_rx_reason_niv_encoding_map,
    dcb26_rx_reason_trill_encoding_map,
    dcb26_rx_reason_nat_encoding_map,
    NULL
};

static _shr_rx_decap_tunnel_t dcb32_rx_decap_tunnel_map[] = {
    _SHR_RX_DECAP_NONE,
    _SHR_RX_DECAP_ACCESS_SVP,
    _SHR_RX_DECAP_MIM,
    _SHR_RX_DECAP_L2GRE,
    _SHR_RX_DECAP_VXLAN,
    _SHR_RX_DECAP_AMT,
    _SHR_RX_DECAP_IP,
    _SHR_RX_DECAP_TRILL,
    _SHR_RX_DECAP_L2MPLS_1LABEL,
    _SHR_RX_DECAP_L2MPLS_2LABEL,
    _SHR_RX_DECAP_L2MPLS_1LABELCW,
    _SHR_RX_DECAP_L2MPLS_2LABELCW,
    _SHR_RX_DECAP_L3MPLS_1LABEL,
    _SHR_RX_DECAP_L3MPLS_2LABEL,
    _SHR_RX_DECAP_L3MPLS_1LABELCW,
    _SHR_RX_DECAP_L3MPLS_2LABELCW
};
static uint32 dcb32_rx_decap_tunnel_get(dcb_t *dcb) {
    dcb32_t *d = (dcb32_t *)dcb;
    /* Check for array bounds */
    if ( (d->decap_tunnel_type <
    (sizeof (dcb32_rx_decap_tunnel_map) / sizeof (_shr_rx_decap_tunnel_t))) ) {
        return (uint32)dcb32_rx_decap_tunnel_map[d->decap_tunnel_type];
    }
    return (uint32)_SHR_RX_DECAP_NONE;
}

/* Fields extracted from MH/PBI */
GETFUNCFIELD(32, rx_ingport, srcport)

GETFUNCFIELD(32, rx_cos, cos)
GETFUNCEXPR(32, rx_mirror, ((SOC_CPU_OPCODE_TYPE_IP_0 == d->cpu_opcode_type) ?
                            (d->reason & (1 << 17)) : 0))
GETFUNCFIELD(32, outer_vid, outer_vid)
GETFUNCFIELD(32, rx_switch_drop, uc_switch_drop)
GETFUNCNULL(32, olp_encap_oam_pkt)
GETFUNCNULL(32, read_ecc_error)
GETFUNCNULL(32, desc_remaining)
SETFUNCNULL(32, desc_remaining)
GETFUNCNULL(32, desc_status)
SETFUNCNULL(32, desc_status)

static void
dcb32_desc_intr_set(dcb_t * dcb, int val)
{
    dcb32_t * d = (dcb32_t *)dcb;

    d->c_intr = val;
    return;
}

static uint32
dcb32_desc_intr_get(dcb_t * dcb)
{
    dcb32_t * d = (dcb32_t *)dcb;
    return (d->c_intr);
}

/*
 * DCB Type 32 Support
 */
dcb_op_t dcb32_op = {
    32,
    sizeof(dcb32_t),
    dcb32_rx_reason_maps,
    dcb0_rx_reason_map_get,
    dcb21_rx_reasons_get,
    dcb19_init,
    dcb19_addtx,
    dcb19_addrx,
    dcb19_intrinfo,
    dcb19_reqcount_set,
    dcb19_reqcount_get,
    dcb26_xfercount_get,
    dcb0_addr_set,
    dcb0_addr_get,
    dcb0_paddr_get,
    dcb19_done_set,
    dcb19_done_get,
    dcb19_sg_set,
    dcb19_sg_get,
    dcb19_chain_set,
    dcb19_chain_get,
    dcb19_reload_set,
    dcb19_reload_get,
    dcb32_desc_intr_set,
    dcb32_desc_intr_get,
    dcb19_tx_l2pbm_set,
    dcb19_tx_utpbm_set,
    dcb19_tx_l3pbm_set,
    dcb19_tx_crc_set,
    dcb19_tx_cos_set,
    dcb19_tx_destmod_set,
    dcb19_tx_destport_set,
    dcb19_tx_opcode_set,
    dcb19_tx_srcmod_set,
    dcb19_tx_srcport_set,
    dcb19_tx_prio_set,
    dcb19_tx_pfm_set,
    dcb26_rx_untagged_get,
    dcb19_rx_crc_get,
    dcb32_rx_cos_get,
    dcb26_rx_destmod_get,
    dcb26_rx_destport_get,
    dcb26_rx_opcode_get,
    dcb26_rx_classtag_get,
    dcb26_rx_matchrule_get,
    dcb19_rx_start_get,
    dcb19_rx_end_get,
    dcb19_rx_error_get,
    dcb26_rx_prio_get,
    dcb26_rx_reason_get,
    dcb26_rx_reason_hi_get,
    dcb32_rx_ingport_get,
    dcb26_rx_srcport_get,
    dcb26_rx_srcmod_get,
    dcb26_rx_mcast_get,
    dcb26_rx_vclabel_get,
    dcb32_rx_mirror_get,
    dcb26_rx_timestamp_get,
    dcb26_rx_timestamp_upper_get,
    dcb19_hg_set,
    dcb19_hg_get,
    dcb19_stat_set,
    dcb19_stat_get,
    dcb19_purge_set,
    dcb19_purge_get,
    dcb26_mhp_get,
    dcb32_outer_vid_get,
    dcb26_outer_pri_get,
    dcb26_outer_cfi_get,
    dcb23_rx_outer_tag_action_get,
    dcb26_inner_vid_get,
    dcb26_inner_pri_get,
    dcb26_inner_cfi_get,
    dcb23_rx_inner_tag_action_get,
    dcb26_rx_bpdu_get,
    dcb26_rx_l3_intf_get,
    dcb32_rx_decap_tunnel_get,
    dcb32_rx_switch_drop_get,
    dcb32_olp_encap_oam_pkt_get,
    dcb32_read_ecc_error_get,
    dcb32_desc_remaining_get,
    dcb32_desc_remaining_set,
    dcb32_desc_status_get,
    dcb32_desc_status_set,
    dcb32_rx_hg2_ext_eh_type_get,
};
#endif /* BCM_TOMAHAWK_SUPPORT */

#if defined(BCM_TRIDENT2PLUS_SUPPORT)
/*
 * DCB Type 33 Support
 */
/* From FORMAT CPU_OPCODES */
static soc_rx_reason_t
dcb33_rx_reason_map[] = {
    socRxReasonUnknownVlan,        /*  0: CPU_UVLAN */
    socRxReasonL2SourceMiss,       /*  1: CPU_SLF */
    socRxReasonL2DestMiss,         /*  2: CPU_DLF */
    socRxReasonL2Move,             /*  3: CPU_L2MOVE */
    socRxReasonL2Cpu,              /*  4: CPU_L2CPU */
    socRxReasonSampleSource,       /*  5: CPU_SFLOW_SRC */
    socRxReasonSampleDest,         /*  6: CPU_SFLOW_DST */
    socRxReasonL3SourceMiss,       /*  7: CPU_L3SRC_MISS */
    socRxReasonL3DestMiss,         /*  8: CPU_L3DST_MISS */
    socRxReasonL3SourceMove,       /*  9: CPU_L3SRC_MOVE */
    socRxReasonMcastMiss,          /* 10: CPU_MC_MISS */
    socRxReasonIpMcastMiss,        /* 11: CPU_IPMC_MISS */
    socRxReasonFilterMatch,        /* 12: CPU_FFP */
    socRxReasonL3HeaderError,      /* 13: CPU_L3HDR_ERR */
    socRxReasonProtocol,           /* 14: CPU_PROTOCOL_PKT */
    socRxReasonDosAttack,          /* 15: CPU_DOS_ATTACK */
    socRxReasonMartianAddr,        /* 16: CPU_MARTIAN_ADDR */
    socRxReasonTunnelError,        /* 17: CPU_TUNNEL_ERR */
    socRxReasonFailoverDrop,       /* 18: FAILOVER_DROP */
    socRxReasonIcmpRedirect,       /* 19: ICMP_REDIRECT */
    socRxReasonL3Slowpath,         /* 20: L3_SLOWPATH */
    socRxReasonParityError,        /* 21: PARITY_ERROR */
    socRxReasonL3MtuFail,          /* 22: L3_MTU_CHECK_FAIL */
    socRxReasonHigigHdrError,      /* 23: HGHDR_ERROR */
    socRxReasonMcastIdxError,      /* 24: MCIDX_ERROR */
    socRxReasonVlanFilterMatch,    /* 25: VFP */
    socRxReasonClassBasedMove,     /* 26: CBSM_PREVENTED */
    socRxReasonL3AddrBindFail,     /* 27: MAC_BIND_FAIL */
    socRxReasonMplsLabelMiss,      /* 28: MPLS_LABEL_MISS */
    socRxReasonMplsInvalidAction,  /* 29: MPLS_INVALID_ACTION */
    socRxReasonMplsInvalidPayload, /* 30: MPLS_INVALID_PAYLOAD */
    socRxReasonMplsTtl,            /* 31: MPLS_TTL_CHECK_FAIL */
    socRxReasonMplsSequenceNumber, /* 32: MPLS_SEQ_NUM_FAIL */
    socRxReasonL2NonUnicastMiss,   /* 33: PBT_NONUC_PKT */
    socRxReasonNhop,               /* 34: L3_NEXT_HOP */
    socRxReasonMplsUnknownAch,     /* 35: MPLS_UNKNOWN_ACH_ERROR */
    socRxReasonStation,            /* 36: MY_STATION */
    socRxReasonNiv,                /* 37: NIV_DROP_REASON_ENCODING */
    socRxReasonNiv,                /* 38: ->  */
    socRxReasonNiv,                /* 39: 3-bit */
    socRxReasonVlanTranslate,      /* 40: VXLT_MISS */
    socRxReasonTimeSync,           /* 41: TIME_SYNC */
    socRxReasonOAMSlowpath,        /* 42: OAM_SLOWPATH */
    socRxReasonOAMError,           /* 43: OAM_ERROR */
    socRxReasonTrill,              /* 44: TRILL_DROP_REASON_ENCODING */
    socRxReasonTrill,              /* 45: -> */
    socRxReasonTrill,              /* 46: 3-bit */
    socRxReasonL2GreSipMiss,       /* 47: L2GRE_SIP_MISS */
    socRxReasonL2GreVpnIdMiss,     /* 48: L2GRE_VPNID_MISS */
    socRxReasonBfdSlowpath,        /* 49: BFD_SLOWPATH */
    socRxReasonBfd,                /* 50: BFD_ERROR */
    socRxReasonOAMLMDM,            /* 51: OAM_LMDM */
    socRxReasonCongestionCnm,      /* 52: ICNM */
    socRxReasonMplsIllegalReservedLabel, /* 53: MPLS_ILLEGAL_RESERVED_LABEL */
    socRxReasonMplsRouterAlertLabel, /* 54: MPLS_ALERT_LABEL */
    socRxReasonCongestionCnmProxy, /* 55: QCN_CNM_PRP */
    socRxReasonCongestionCnmProxyError, /* 56: QCN_CNM_PRP_DLF */
    socRxReasonVxlanSipMiss,       /* 57: VXLAN_SIP_MISS */
    socRxReasonVxlanVpnIdMiss,     /* 58: VXLAN_VN_ID_MISS */
    socRxReasonFcoeZoneCheckFail,  /* 59: FCOE_ZONE_CHECK_FAIL */
    socRxReasonNat,                /* 60: NAT_DROP_REASON_ENCODING */
    socRxReasonNat,                /* 61: -> */
    socRxReasonNat,                /* 62: 3-bit */
    socRxReasonIpmcInterfaceMismatch /* 63: CPU_IPMC_INTERFACE_MISMATCH */
};

/* From FORMAT NIV_CPU_OPCODE_ENCODING  */
static soc_rx_reason_t dcb33_rx_reason_niv_encoding_map[] = {
    socRxReasonNiv,                  /* 0: NO_ERRORS
                                      * Base field, must match the entries above */
    socRxReasonNivPrioDrop,          /* 1:DOT1P_ADMITTANCE_DISCARD */
    socRxReasonNivInterfaceMiss,     /* 2:VIF_LOOKUP_MISS */
    socRxReasonNivRpfFail,           /* 3:RPF_LOOKUP_MISS */
    socRxReasonNivTagInvalid,        /* 4:VNTAG_FORMAT_ERROR */
    socRxReasonNivTagDrop,           /* 5:VNTAG_PRESENT_DROP */
    socRxReasonNivUntagDrop,         /* 6:VNTAG_NOT_PRESENT_DROP */
    socRxReasonUnknownSubtendingPort /* 7:UNKOWN SUBTENDING PORT */
};

/* From FORMAT NAT_CPU_OPCODE_ENCODING */
static soc_rx_reason_t dcb33_rx_reason_nat_encoding_map[] = {
    socRxReasonNat,                /* 0:NOP
                                    * Base field, must match the entries above */
    socRxReasonTcpUdpNatMiss,      /* 1:NORMAL */
    socRxReasonIcmpNatMiss,        /* 2:ICMP */
    socRxReasonNatFragment,        /* 3:FRAGMEMT */
    socRxReasonNatMiss,            /* 4:OTHER */
    socRxReasonInvalid,            /* 5:NOT DEFINED */
    socRxReasonInvalid,            /* 6:NOT DEFINED */
#if 1
    socRxReasonInvalid             /* 7:NOT DEFINED */
#endif
};

static soc_rx_reason_t *dcb33_rx_reason_maps[] = {
    dcb33_rx_reason_map,
    dcb33_rx_reason_niv_encoding_map,
    dcb26_rx_reason_trill_encoding_map,
    dcb33_rx_reason_nat_encoding_map,
    NULL
};

static _shr_rx_decap_tunnel_t dcb33_rx_decap_tunnel_map[] = {
    _SHR_RX_DECAP_NONE,
    _SHR_RX_DECAP_ACCESS_SVP,
    _SHR_RX_DECAP_MIM,
    _SHR_RX_DECAP_L2GRE,
    _SHR_RX_DECAP_VXLAN,
    _SHR_RX_DECAP_AMT,
    _SHR_RX_DECAP_IP,
    _SHR_RX_DECAP_TRILL,
    _SHR_RX_DECAP_MPLS_1LABELCW,
    _SHR_RX_DECAP_MPLS_2LABELCW,
    _SHR_RX_DECAP_MPLS_1LABEL,
    _SHR_RX_DECAP_MPLS_2LABEL,
    _SHR_RX_DECAP_L3MPLS_1LABEL,
    _SHR_RX_DECAP_L3MPLS_2LABEL,
    _SHR_RX_DECAP_L3MPLS_1LABELCW,
    _SHR_RX_DECAP_L3MPLS_2LABELCW
}; 
static uint32 dcb33_rx_decap_tunnel_get(dcb_t *dcb) {
    dcb33_t *d = (dcb33_t *)dcb;
    /* Check for array bounds */
    if ( (d->word3.overlay1.decap_tunnel_type <  
    (sizeof (dcb33_rx_decap_tunnel_map) / sizeof (_shr_rx_decap_tunnel_t))) ) {
        return (uint32)dcb33_rx_decap_tunnel_map[d->word3.overlay1.decap_tunnel_type];
    }
    return (uint32)_SHR_RX_DECAP_NONE;
}
GETFUNCFIELD(33, outer_vid, word4.overlay1.outer_vid)

static void
dcb33_desc_intr_set(dcb_t * dcb, int val)
{
    dcb33_t * d = (dcb33_t *)dcb;

    d->c_intr = val;
    return;
}

static uint32
dcb33_desc_intr_get(dcb_t * dcb)
{
    dcb33_t * d = (dcb33_t *)dcb;
    return (d->c_intr);
}

dcb_op_t dcb33_op = {
    33,
    sizeof(dcb33_t),
    dcb33_rx_reason_maps,
    dcb0_rx_reason_map_get,
    dcb21_rx_reasons_get,
    dcb19_init,
    dcb19_addtx,
    dcb19_addrx,
    dcb19_intrinfo,
    dcb19_reqcount_set,
    dcb19_reqcount_get,
    dcb26_xfercount_get,
    dcb0_addr_set,
    dcb0_addr_get,
    dcb0_paddr_get,
    dcb19_done_set,
    dcb19_done_get,
    dcb19_sg_set,
    dcb19_sg_get,
    dcb19_chain_set,
    dcb19_chain_get,
    dcb19_reload_set,
    dcb19_reload_get,
    dcb33_desc_intr_set,
    dcb33_desc_intr_get,
    dcb19_tx_l2pbm_set,
    dcb19_tx_utpbm_set,
    dcb19_tx_l3pbm_set,
    dcb19_tx_crc_set,
    dcb19_tx_cos_set,
    dcb19_tx_destmod_set,
    dcb19_tx_destport_set,
    dcb19_tx_opcode_set,
    dcb19_tx_srcmod_set,
    dcb19_tx_srcport_set,
    dcb19_tx_prio_set,
    dcb19_tx_pfm_set,
    dcb26_rx_untagged_get,
    dcb19_rx_crc_get,
    dcb26_rx_cos_get,
    dcb26_rx_destmod_get,
    dcb26_rx_destport_get,
    dcb26_rx_opcode_get,
    dcb26_rx_classtag_get,
    dcb26_rx_matchrule_get,
    dcb19_rx_start_get,
    dcb19_rx_end_get,
    dcb19_rx_error_get,
    dcb26_rx_prio_get,
    dcb26_rx_reason_get,
    dcb26_rx_reason_hi_get,
    dcb26_rx_ingport_get,
    dcb26_rx_srcport_get,
    dcb26_rx_srcmod_get,
    dcb26_rx_mcast_get,
    dcb26_rx_vclabel_get,
    dcb23_rx_mirror_get,
    dcb26_rx_timestamp_get,
    dcb26_rx_timestamp_upper_get,
    dcb19_hg_set,
    dcb19_hg_get,
    dcb19_stat_set,
    dcb19_stat_get,
    dcb19_purge_set,
    dcb19_purge_get,
    dcb26_mhp_get,
    dcb33_outer_vid_get,
    dcb26_outer_pri_get,
    dcb26_outer_cfi_get,
    dcb23_rx_outer_tag_action_get,
    dcb26_inner_vid_get,
    dcb26_inner_pri_get,
    dcb26_inner_cfi_get,
    dcb23_rx_inner_tag_action_get,
    dcb26_rx_bpdu_get,
    dcb26_rx_l3_intf_get,
    dcb33_rx_decap_tunnel_get,
    dcb23_rx_switch_drop_get,
    dcb23_olp_encap_oam_pkt_get,
    dcb23_read_ecc_error_get,
    dcb23_desc_remaining_get,
    dcb23_desc_remaining_set,
    dcb23_desc_status_get,
    dcb23_desc_status_set,
    NULL,
};
#endif /* BCM_TRIDENT2PLUS_SUPPORT */


#ifdef BCM_APACHE_SUPPORT

/* From FORMAT CPU_OPCODES */
static soc_rx_reason_t
dcb35_rx_reason_map_ip[] = {
    socRxReasonUnknownVlan,        /*  0: CPU_UVLAN */
    socRxReasonL2SourceMiss,       /*  1: CPU_SLF */
    socRxReasonL2DestMiss,         /*  2: CPU_DLF */
    socRxReasonL2Move,             /*  3: CPU_L2MOVE */
    socRxReasonL2Cpu,              /*  4: CPU_L2CPU */
    socRxReasonSampleSource,       /*  5: CPU_SFLOW_SRC */
    socRxReasonSampleDest,         /*  6: CPU_SFLOW_DST */
    socRxReasonL3SourceMiss,       /*  7: CPU_L3SRC_MISS */
    socRxReasonL3DestMiss,         /*  8: CPU_L3DST_MISS */
    socRxReasonL3SourceMove,       /*  9: CPU_L3SRC_MOVE */
    socRxReasonMcastMiss,          /* 10: CPU_MC_MISS */
    socRxReasonIpMcastMiss,        /* 11: CPU_IPMC_MISS */
    socRxReasonFilterMatch,        /* 12: CPU_FFP */
    socRxReasonL3HeaderError,      /* 13: CPU_L3HDR_ERR */
    socRxReasonProtocol,           /* 14: CPU_PROTOCOL_PKT */
    socRxReasonDosAttack,          /* 15: CPU_DOS_ATTACK */
    socRxReasonMartianAddr,        /* 16: CPU_MARTIAN_ADDR */
    socRxReasonTunnelError,        /* 17: CPU_TUNNEL_ERR */
    socRxReasonFailoverDrop,       /* 18: FAILOVER_DROP */
    socRxReasonIcmpRedirect,       /* 19: ICMP_REDIRECT */
    socRxReasonL3Slowpath,         /* 20: L3_SLOWPATH */
    socRxReasonParityError,        /* 21: PARITY_ERROR */
    socRxReasonL3MtuFail,          /* 22: L3_MTU_CHECK_FAIL */
    socRxReasonHigigHdrError,      /* 23: HGHDR_ERROR */
    socRxReasonMcastIdxError,      /* 24: MCIDX_ERROR */
    socRxReasonVlanFilterMatch,    /* 25: VFP */
    socRxReasonClassBasedMove,     /* 26: CBSM_PREVENTED */
    socRxReasonL3AddrBindFail,     /* 27: MAC_BIND_FAIL */
    socRxReasonMplsLabelMiss,      /* 28: MPLS_LABEL_MISS */
    socRxReasonMplsInvalidAction,  /* 29: MPLS_INVALID_ACTION */
    socRxReasonMplsInvalidPayload, /* 30: MPLS_INVALID_PAYLOAD */
    socRxReasonMplsTtl,            /* 31: MPLS_TTL_CHECK_FAIL */
    socRxReasonMplsSequenceNumber, /* 32: MPLS_SEQ_NUM_FAIL */
    socRxReasonL2NonUnicastMiss,   /* 33: PBT_NONUC_PKT */
    socRxReasonNhop,               /* 34: L3_NEXT_HOP */
    socRxReasonMplsUnknownAch,     /* 35: MPLS_UNKNOWN_ACH_ERROR */
    socRxReasonStation,            /* 36: MY_STATION */
    socRxReasonNiv,                /* 37: NIV_DROP_REASON_ENCODING */
    socRxReasonNiv,                /* 38: ->  */
    socRxReasonNiv,                /* 39: 3-bit */
    socRxReasonVlanTranslate,      /* 40: VXLT_MISS */
    socRxReasonTimeSync,           /* 41: TIME_SYNC */
    socRxReasonOAMSlowpath,        /* 42: OAM_SLOWPATH */
    socRxReasonMplsLookupsExceeded,/* 43: MPLS_OUT_OF_LOOKUPS */
    socRxReasonTrill,              /* 44: TRILL_DROP_REASON_ENCODING */
    socRxReasonTrill,              /* 45: -> */
    socRxReasonTrill,              /* 46: 3-bit */
    socRxReasonL2GreSipMiss,       /* 47: L2GRE_SIP_MISS */
    socRxReasonL2GreVpnIdMiss,     /* 48: L2GRE_VPNID_MISS */
    socRxReasonBfdSlowpath,        /* 49: BFD_SLOWPATH */
    socRxReasonBfd,                /* 50: BFD_ERROR */
    socRxReasonMplsReservedEntropyLabel,/* 51: ENTROPY_LABEL_IN_UNALLOWED_RANGE */
    socRxReasonCongestionCnm,      /* 52: ICNM */
    socRxReasonMplsIllegalReservedLabel, /* 53: MPLS_ILLEGAL_RESERVED_LABEL */
    socRxReasonMplsRouterAlertLabel, /* 54: MPLS_ALERT_LABEL */
    socRxReasonCongestionCnmProxy, /* 55: QCN_CNM_PRP */
    socRxReasonCongestionCnmProxyError, /* 56: QCN_CNM_PRP_DLF */
    socRxReasonVxlanSipMiss,       /* 57: VXLAN_SIP_MISS */
    socRxReasonVxlanVpnIdMiss,     /* 58: VXLAN_VN_ID_MISS */
    socRxReasonFcoeZoneCheckFail,  /* 59: FCOE_ZONE_CHECK_FAIL */
    socRxReasonNat,                /* 60: NAT_DROP_REASON_ENCODING */
    socRxReasonNat,                /* 61: -> */
    socRxReasonNat,                /* 62: 3-bit */
    socRxReasonIpmcInterfaceMismatch /* 63: CPU_IPMC_INTERFACE_MISMATCH */
};

/* From format EP_CPU_OPCODES */
static soc_rx_reason_t
dcb35_rx_reason_map_ep[] = {
    socRxReasonSplitHorizon,       /* 0: CPUE_PRUNE */
    socRxReasonNivPrune,           /* 1: CPUE_NIV_DISCARD */
    socRxReasonVirtualPortPrune,   /* 2: CPUE_SPLIT_HORIZON */
    socRxReasonHigigHdrError,      /* 3: CPUE_HIGIG */
    socRxReasonTrillPacketPortMismatch, /* 4: CPUE_TRILL */
    socRxReasonL2MtuFail,          /* 5: CPUE_MTU */
    socRxReasonInvalid,            /* 6: CPUE_PKT_SMALL_OR_BIG */
    socRxReasonUnknownVlan,        /* 7: CPUE_VLAN */
    socRxReasonStp,                /* 8: CPUE_STG */
    socRxReasonVlanTranslate,      /* 9: CPUE_VXLT */
    socRxReasonNonUnicastDrop,     /* 10: CPUE_MULTI_DEST */
    socRxReasonInvalid,            /* 11: CPUE_CNM */
    socRxReasonInvalid,            /* 12: CPUE_FCOE */
    socRxReasonInvalid,            /* 13: CPUE_CFI */
    socRxReasonTtl,                /* 14: CPUE_TTL_DROP */
    socRxReasonIpmc,               /* 15: CPUE_L3ERR */
    socRxReasonTunnelError,        /* 16: CPUE_TUNNEL */
    socRxReasonFilterMatch,        /* 17: CPUE_EFP */
    socRxReasonInvalid,            /* 18: CPUE_PROT */
    socRxReasonInvalid,            /* 19: CPUE_1588 */
    socRxReasonInvalid,            /* 20: CPUE_COMPOSITE_ERROR */
    socRxReasonInvalid,            /* 21: EP_DROP_ORIG */

    socRxReasonInvalid,            /* Offset 22 */
    socRxReasonInvalid,            /* Offset 23 */
    socRxReasonInvalid,            /* Offset 24 */
    socRxReasonInvalid,            /* Offset 25 */
    socRxReasonInvalid,            /* Offset 26 */
    socRxReasonInvalid,            /* Offset 27 */
    socRxReasonInvalid,            /* Offset 28 */
    socRxReasonInvalid,            /* Offset 29 */
    socRxReasonInvalid,            /* Offset 30 */
    socRxReasonInvalid,            /* Offset 31 */
    socRxReasonInvalid,            /* Offset 32 */
    socRxReasonInvalid,            /* Offset 33 */
    socRxReasonInvalid,            /* Offset 34 */
    socRxReasonInvalid,            /* Offset 35 */
    socRxReasonInvalid,            /* Offset 36 */
    socRxReasonInvalid,            /* Offset 37 */
    socRxReasonInvalid,            /* Offset 38 */
    socRxReasonInvalid,            /* Offset 39 */
    socRxReasonInvalid,            /* Offset 40 */
    socRxReasonInvalid,            /* Offset 41 */
    socRxReasonInvalid,            /* Offset 42 */
    socRxReasonInvalid,            /* Offset 43 */
    socRxReasonInvalid,            /* Offset 44 */
    socRxReasonInvalid,            /* Offset 45 */
    socRxReasonInvalid,            /* Offset 46 */
    socRxReasonInvalid,            /* Offset 47 */
    socRxReasonInvalid,            /* Offset 48 */
    socRxReasonInvalid,            /* Offset 49 */
    socRxReasonInvalid,            /* Offset 50 */
    socRxReasonInvalid,            /* Offset 51 */
    socRxReasonInvalid,            /* Offset 52 */
    socRxReasonInvalid,            /* Offset 53 */
    socRxReasonInvalid,            /* Offset 54 */
    socRxReasonInvalid,            /* Offset 55 */
    socRxReasonInvalid,            /* Offset 56 */
    socRxReasonInvalid,            /* Offset 57 */
    socRxReasonInvalid,            /* Offset 58 */
    socRxReasonInvalid,            /* Offset 59 */
    socRxReasonInvalid,            /* Offset 60 */
    socRxReasonInvalid,            /* Offset 61 */
    socRxReasonInvalid,            /* Offset 62 */
    socRxReasonInvalid             /* Offset 63 */
};

static soc_rx_reason_t *dcb35_rx_reason_maps[4][5] = {
    {   /* IP_0 */
        dcb35_rx_reason_map_ip,
        dcb33_rx_reason_niv_encoding_map,
        dcb26_rx_reason_trill_encoding_map,
        dcb33_rx_reason_nat_encoding_map,
        NULL
    },
    {NULL}, /* Index 1 is invalid */
    {   /*  EP */
        dcb35_rx_reason_map_ep,
        NULL
    },
    {NULL} /* Index 3 is invalid */
};


/*
 * Function:
 *      dcb35_rx_reason_map_get
 * Purpose:
 *      Return the RX reason map for DCB 35 type.
 * Parameters:
 *      dcb_op - DCB operations
 *      dcb    - dma control block
 * Returns:
 *      RX reason map pointer
 */
static soc_rx_reason_t *
dcb35_rx_reason_map_get(dcb_op_t *dcb_op, dcb_t *dcb)
{
    soc_rx_reason_t *map = NULL;
    dcb35_t * d = (dcb35_t *)dcb;

    map = dcb35_rx_reason_maps[(d->word4.overlay1.cpu_opcode_type)][0];

    return map;
}

static _shr_rx_decap_tunnel_t dcb35_rx_decap_tunnel_map[] = {
    _SHR_RX_DECAP_NONE,
    _SHR_RX_DECAP_ACCESS_SVP,
    _SHR_RX_DECAP_MIM,
    _SHR_RX_DECAP_L2GRE,
    _SHR_RX_DECAP_VXLAN,
    _SHR_RX_DECAP_AMT,
    _SHR_RX_DECAP_IP,
    _SHR_RX_DECAP_TRILL,
    _SHR_RX_DECAP_MPLS_1LABELCW,
    _SHR_RX_DECAP_MPLS_2LABELCW,
    _SHR_RX_DECAP_MPLS_3LABELCW,
    _SHR_RX_DECAP_MPLS_3LABEL_ENTROPYCW,
    _SHR_RX_DECAP_MPLS_1LABEL,
    _SHR_RX_DECAP_MPLS_2LABEL,
    _SHR_RX_DECAP_MPLS_3LABEL,
    _SHR_RX_DECAP_MPLS_3LABEL_ENTROPY
};
static uint32 dcb35_rx_decap_tunnel_get(dcb_t *dcb) {
    dcb35_t *d = (dcb35_t *)dcb;
    /* Check for array bounds */
    if ( (d->word3.overlay1.decap_tunnel_type <
    (sizeof (dcb35_rx_decap_tunnel_map) / sizeof (_shr_rx_decap_tunnel_t))) ) {
        return (uint32)dcb35_rx_decap_tunnel_map[d->word3.overlay1.decap_tunnel_type];
    }
    return (uint32)_SHR_RX_DECAP_NONE;
}

GETFUNCFIELD(35, olp_encap_oam_pkt, olp_encap_oam_pkt)
GETFUNCNULL(35, read_ecc_error)
GETFUNCNULL(35, desc_remaining)
SETFUNCNULL(35, desc_remaining)
GETFUNCNULL(35, desc_status)
SETFUNCNULL(35, desc_status)

dcb_op_t dcb35_op = {
    35,
    sizeof(dcb35_t),
    dcb35_rx_reason_maps[0],
    dcb35_rx_reason_map_get,
    dcb21_rx_reasons_get,
    dcb19_init,
    dcb19_addtx,
    dcb19_addrx,
    dcb19_intrinfo,
    dcb19_reqcount_set,
    dcb19_reqcount_get,
    dcb26_xfercount_get,
    dcb0_addr_set,
    dcb0_addr_get,
    dcb0_paddr_get,
    dcb19_done_set,
    dcb19_done_get,
    dcb19_sg_set,
    dcb19_sg_get,
    dcb19_chain_set,
    dcb19_chain_get,
    dcb19_reload_set,
    dcb19_reload_get,
    dcb33_desc_intr_set,
    dcb33_desc_intr_get,
    dcb19_tx_l2pbm_set,
    dcb19_tx_utpbm_set,
    dcb19_tx_l3pbm_set,
    dcb19_tx_crc_set,
    dcb19_tx_cos_set,
    dcb19_tx_destmod_set,
    dcb19_tx_destport_set,
    dcb19_tx_opcode_set,
    dcb19_tx_srcmod_set,
    dcb19_tx_srcport_set,
    dcb19_tx_prio_set,
    dcb19_tx_pfm_set,
    dcb26_rx_untagged_get,
    dcb19_rx_crc_get,
    dcb26_rx_cos_get,
    dcb26_rx_destmod_get,
    dcb26_rx_destport_get,
    dcb26_rx_opcode_get,
    dcb26_rx_classtag_get,
    dcb26_rx_matchrule_get,
    dcb19_rx_start_get,
    dcb19_rx_end_get,
    dcb19_rx_error_get,
    dcb26_rx_prio_get,
    dcb26_rx_reason_get,
    dcb26_rx_reason_hi_get,
    dcb26_rx_ingport_get,
    dcb26_rx_srcport_get,
    dcb26_rx_srcmod_get,
    dcb26_rx_mcast_get,
    dcb26_rx_vclabel_get,
    dcb23_rx_mirror_get,
    dcb26_rx_timestamp_get,
    dcb26_rx_timestamp_upper_get,
    dcb19_hg_set,
    dcb19_hg_get,
    dcb19_stat_set,
    dcb19_stat_get,
    dcb19_purge_set,
    dcb19_purge_get,
    dcb26_mhp_get,
    dcb33_outer_vid_get,
    dcb26_outer_pri_get,
    dcb26_outer_cfi_get,
    dcb23_rx_outer_tag_action_get,
    dcb26_inner_vid_get,
    dcb26_inner_pri_get,
    dcb26_inner_cfi_get,
    dcb23_rx_inner_tag_action_get,
    dcb26_rx_bpdu_get,
    dcb26_rx_l3_intf_get,
    dcb35_rx_decap_tunnel_get,
    dcb23_rx_switch_drop_get,
    dcb35_olp_encap_oam_pkt_get,
    dcb35_read_ecc_error_get,
    dcb35_desc_remaining_get,
    dcb35_desc_remaining_set,
    dcb35_desc_status_get,
    dcb35_desc_status_set,
    NULL,
};
#endif /* BCM_APACHE_SUPPORT */

#if defined(BCM_TRIDENT3_SUPPORT)
SETFUNCFIELD(36, reqcount, c_count, uint32 count, count)
GETFUNCFIELD(36, reqcount, c_count)
GETFUNCFIELD(36, xfercount, count)
SETFUNCFIELD(36, done, done, int val, val ? 1 : 0)
GETFUNCFIELD(36, done, done)
SETFUNCFIELD(36, sg, c_sg, int val, val ? 1 : 0)
GETFUNCFIELD(36, sg, c_sg)
SETFUNCFIELD(36, chain, c_chain, int val, val ? 1 : 0)
GETFUNCFIELD(36, chain, c_chain)
SETFUNCFIELD(36, reload, c_reload, int val, val ? 1 : 0)
GETFUNCFIELD(36, reload, c_reload)
GETFUNCFIELD(36, rx_start, start)
GETFUNCFIELD(36, rx_end, end)
GETFUNCFIELD(36, rx_error, error)
SETFUNCFIELD(36, hg, c_hg, uint32 hg, hg)
GETFUNCFIELD(36, hg, c_hg)
SETFUNCFIELD(36, purge, c_purge, uint32 purge, purge)
GETFUNCFIELD(36, purge, c_purge)
GETFUNCFIELD(36, read_ecc_error, ecc_error)
GETFUNCFIELD(36, desc_status, c_desc_status)
GETFUNCFIELD(36, desc_intr, c_intr)
GETFUNCFIELD(36, desc_remaining, c_desc)

GETHDRFUNCFIELD(36, rx_timestamp, timestamp)
GETHDRFUNCNULL(36, rx_timestamp_upper)
GETHDRPTREXPR(36, mhp, &(h->mh0))
GETHDRFUNCFIELD(36, outer_vid, outer_vid)
GETHDRFUNCFIELD(36, outer_pri, word6.overlay1.outer_pri)
GETHDRFUNCFIELD(36, outer_cfi, outer_cfi)
GETHDRFUNCFIELD(36, rx_outer_tag_action, otag_action)
GETHDRFUNCFIELD(36, inner_vid, inner_vid)
GETHDRFUNCFIELD(36, inner_pri, word6.overlay1.inner_pri)
GETHDRFUNCFIELD(36, inner_cfi, word6.overlay1.inner_cfi)
GETHDRFUNCFIELD(36, rx_inner_tag_action, itag_action)
GETHDRFUNCFIELD(36, rx_bpdu, bpdu)
GETHDRFUNCFIELD(36, rx_decap_tunnel, decap_tunnel_type)

static uint32 ep_to_cpu_hdr36_rx_crc_get(void *hdr) {
    return 0;
}

static uint32
ep_to_cpu_hdr36_rx_untagged_get(void *hdr, int dt_mode, int ingport_is_hg)
{

    return 0;
}

static void
ep_to_cpu_hdr36_rx_reasons_get(dcb_op_t *dcb_op, void *hdr, soc_rx_reasons_t *reasons)
{
    soc_rx_reason_t *map, *encoding_map;
    uint32 opcode[2], encoding[3];
    uint32 bit_val;
    int word_idx, bit_idx, opcode_idx, map_idx, word_count;
    int enc_bit = 0;

    SOC_RX_REASON_CLEAR_ALL(*reasons);

    opcode[0] = dcb_op->rx_reason_get(hdr);
    opcode[1] = dcb_op->rx_reason_hi_get(hdr);
    word_count = 2;
    sal_memset(encoding, 0, sizeof(encoding));
    map = dcb_op->rx_reason_maps[0];
    for (word_idx = 0; word_idx < word_count; word_idx++) {
        for (bit_idx = 0; bit_idx < 32; bit_idx++) {
            opcode_idx = word_idx * 32 + bit_idx;
            bit_val = opcode[word_idx] & (1 << bit_idx) ? 1 : 0;
            for (map_idx = 0; ; map_idx++) {
                encoding_map = dcb_op->rx_reason_maps[1 + map_idx];
                if (encoding_map == NULL) {
                    break;
                }
                if (map[opcode_idx] != encoding_map[0]) {
                    continue;
                }
                if (opcode_idx == 0 || map[opcode_idx - 1] != encoding_map[0]) {
                    enc_bit = 0;
                }
                encoding[map_idx] |= bit_val << enc_bit;
                enc_bit++;
                break;
            }
            if (encoding_map == NULL && bit_val) {
                SOC_RX_REASON_SET(*reasons, map[opcode_idx]);
            }
        }
    }

    for (map_idx = 0; ; map_idx++) {
        encoding_map = dcb_op->rx_reason_maps[1 + map_idx];
        if (encoding_map == NULL) {
            break;
        }
        if (encoding[map_idx] != socRxReasonInvalid) {
            SOC_RX_REASON_SET(*reasons, encoding_map[encoding[map_idx]]);
        }
    }

    /* BPDU bit should be a reason, paste it in here */
    if (dcb_op->rx_bpdu_get(hdr)) {
        SOC_RX_REASON_SET(*reasons, socRxReasonBpdu);
    }

    return;
}


GETHDRFUNCFIELD(36, rx_cos, word8.overlay1.cpu_cos)
GETHDRHG2FUNCFIELD(36, rx_destmod, dst_mod)
GETHDRHG2FUNCFIELD(36, rx_destport, dst_port)

GETHDRHG2FUNCFIELD(36, rx_opcode, opcode)
GETHDRHG2FUNCEXPR(36, rx_classtag, (h->ppd_overlay1.ppd_type != 1 ? 0 :
                                 (h->ppd_overlay2.ctag_hi << 8) |
                                 (h->ppd_overlay2.ctag_lo)))
GETHDRFUNCFIELD(36, rx_matchrule, match_rule)
GETHDRHG2FUNCFIELD(36, rx_prio, vlan_pri) /* outer_pri */

static uint32
ep_to_cpu_hdr36_rx_reason_get(void *hdr)
{
    ep_to_cpu_hdr36_t *h = (ep_to_cpu_hdr36_t *)hdr;

    return (h->switch_pkt) ? (h->reason) : 0;
}

static uint32
ep_to_cpu_hdr36_rx_reason_hi_get(void *hdr)
{
    ep_to_cpu_hdr36_t *h = (ep_to_cpu_hdr36_t *)hdr;

    return (h->switch_pkt) ? (h->reason_hi) : 0;
}

GETHDRFUNCFIELD(36, rx_ingport, word8.overlay1.srcport)
GETHDRHG2FUNCFIELD(36, rx_srcport, src_port)
GETHDRHG2FUNCFIELD(36, rx_srcmod, src_mod)
GETHDRHG2FUNCEXPR(36, rx_mcast, ((h->ppd_overlay1.dst_mod << 8) |
                              (h->ppd_overlay1.dst_port)))
GETHDRHG2FUNCEXPR(36, rx_vclabel, ((h->ppd_overlay1.vc_label_19_16 << 16) |
                              (h->ppd_overlay1.vc_label_15_8 << 8) |
                              (h->ppd_overlay1.vc_label_7_0)))
GETHDRFUNCEXPR(36, rx_mirror, ((h->switch_pkt) ? 0:1))

GETHDRFUNCEXPR(36, rx_l3_intf, h->word6.overlay1.nhi_b ? \
   (h->word6.overlay1.nhi_b + _SHR_L3_EGRESS_IDX_MIN): \
   (((h->nhi_a_hi << 8) | h->word6.overlay1.nhi_a_lo) + _SHR_L3_EGRESS_IDX_MIN))
GETHDRFUNCNULL(36, rx_switch_drop)
GETHDRFUNCNULL(36, olp_encap_oam_pkt)

#if 0

static void
ep_to_cpu_hdr_funcerr(int dt, char *name)
{
    LOG_ERROR(BSL_LS_SOC_COMMON,
              (BSL_META("ERROR: ep_to_cpu_hdr%d_%s called\n"), dt, name));
}
#endif

static void
dcb36_init(dcb_t *dcb)
{
    uint32      *d = (uint32 *)dcb;

    d[0] = d[1] = d[2] = d[3] = 0;
}

static int
dcb36_add_rx(dv_t *dv, sal_vaddr_t addr, uint32 count, uint32 flags)
{
    dcb36_t     *d;     /* DCB */
    uint32            *di;    /* DCB integer pointer */
    sal_paddr_t        paddr;

    d = (dcb36_t *)SOC_DCB_IDX2PTR(dv->dv_unit, dv->dv_dcb, dv->dv_vcnt);

    if (dv->dv_vcnt > 0) {      /* chain off previous dcb */
        d[-1].c_chain = 1;
    }

    di = (uint32 *)d;
    di[0] = di[1] = di[2] = di[3] = 0;

    if (addr) {
        paddr = soc_cm_l2p(dv->dv_unit, (void *)addr);
        d->addr_lo = PTR_TO_INT(paddr);
        d->addr_hi = (0x10000000 | PTR_HI_TO_INT(paddr));
    }

    d->c_count = count;
    d->c_sg = 1;

    dv->dv_vcnt += 1;
    return dv->dv_cnt - dv->dv_vcnt;
}

static int
dcb36_add_tx(dv_t *dv, sal_vaddr_t addr, uint32 count,
            pbmp_t l2pbm, pbmp_t utpbm, pbmp_t l3pbm, uint32 flags, uint32 *hgh)
{
    dcb36_t     *d;     /* DCB */
    uint32            *di;    /* DCB integer pointer */
    sal_paddr_t       paddr = 0;  /* Packet buffer physical address */

    d = (dcb36_t *)SOC_DCB_IDX2PTR(dv->dv_unit, dv->dv_dcb, dv->dv_vcnt);

    if (addr)
        paddr = soc_cm_l2p(dv->dv_unit, (void *)addr);

    if (dv->dv_vcnt > 0 && (dv->dv_flags & DV_F_COMBINE_DCB) &&
        (d[-1].c_sg != 0) &&
        ((d[-1].addr_lo + d[-1].c_count) == paddr) &&
        (d[-1].c_count + count <= DCB_MAX_REQCOUNT)) {
        d[-1].c_count += count;
        return dv->dv_cnt - dv->dv_vcnt;
    }

    if (dv->dv_vcnt >= dv->dv_cnt) {
        return SOC_E_FULL;
    }
    if (dv->dv_vcnt > 0) {      /* chain off previous dcb */
        d[-1].c_chain = 1;
    }

    di = (uint32 *)d;
    di[0] = di[1] = di[2] = di[3] = 0;

    d->addr_lo = PTR_TO_INT(paddr);
    d->addr_hi = (0x10000000 | PTR_HI_TO_INT(paddr));
    d->c_count = count;
    d->c_sg = 1;

    d->c_purge = SOC_DMA_PURGE_GET(flags);

    if (SOC_DMA_HG_GET(flags)) {
        d->c_hg = 1;
    }

    dv->dv_vcnt += 1;

    return dv->dv_cnt - dv->dv_vcnt;
}

static uint32
dcb36_intr_info(int unit, dcb_t *dcb, int tx, uint32 *count)
{
    dcb36_t      *d = (dcb36_t *)dcb;     /*  DCB */
    uint32      f;                                    /* SOC_DCB_INFO_* flags */

    if (!d->done) {
        return 0;
    }

    f = SOC_DCB_INFO_DONE;
    if (tx) {
        if (!d->c_sg) {
            f |= SOC_DCB_INFO_PKTEND;
        }
    } else {
        if (d->end) {
            f |= SOC_DCB_INFO_PKTEND;
        }
    }

    *count = d->count;
    return f;
}

/* the addr related functions are the same for all dcb types */
static void
dcb36_addr_set(int unit, dcb_t *dcb, sal_vaddr_t addr)
{
    dcb36_t      *d = (dcb36_t *)dcb;     /*  DCB */
    sal_paddr_t        paddr;

    if (addr == 0) {
        d->addr_lo = 0;
        d->addr_hi = 0;
    } else {
        paddr = soc_cm_l2p(unit, (void *)addr);
        d->addr_lo = PTR_TO_INT(paddr);
        d->addr_hi = (0x10000000 | PTR_HI_TO_INT(paddr));
    }
}

static sal_vaddr_t
dcb36_addr_get(int unit, dcb_t *dcb)
{
    dcb36_t      *d = (dcb36_t *)dcb;     /*  DCB */
    sal_paddr_t        paddr;

#if defined(COMPILER_OVERRIDE_NO_LONGLONG) || defined(__PEDANTIC__)
    paddr = d->addr_lo;
#else
    paddr = (((uint64)(d->addr_hi & ~(0x10000000)) << 32) | d->addr_lo);
#endif

    return (sal_vaddr_t)soc_cm_p2l(unit, paddr);
}

static sal_paddr_t
dcb36_paddr_get(dcb_t *dcb)
{
    uint32      *d = (uint32 *)dcb;

    return (sal_paddr_t)*d;
}

static void
dcb36_desc_intr_set(dcb_t * dcb, int val)
{
    dcb36_t * d = (dcb36_t *)dcb;

    d->c_intr = val;
    return;
}

static uint32
dcb36_desc_status_set(dcb_t * dcb, uint32 val)
{
    dcb36_t * d = (dcb36_t *)dcb;

    d->c_desc_status = val;
    return 0;
}

static uint32
dcb36_desc_remaining_set(dcb_t * dcb, uint32 val)
{
    dcb36_t * d = (dcb36_t *)dcb;

    d->c_desc = val;
    return 0;
}

/*
 * DCB Type 36 Support
 */
/* From FORMAT CPU_OPCODES */
static soc_rx_reason_t
dcb36_rx_reason_map[] = {
    socRxReasonFilterMatch,        /*  0: CPU_FFP */
    socRxReasonVlanFilterMatch,    /*  1: VFP */
    socRxReasonSampleSourceFlex,   /*  2: CPU_SFLOW_FLEX */
    socRxReasonSampleSource,       /*  3: CPU_SFLOW_SRC */
    socRxReasonSampleDest,         /*  4: CPU_SFLOW_DST */
    socRxReasonL2Move,             /*  5: CPU_L2MOVE */
    socRxReasonL2Cpu,              /*  6: CPU_L2CPU */
    socRxReasonL3SourceMiss,       /*  7: CPU_L3SRC_MISS */
    socRxReasonL3DestMiss,         /*  8: CPU_L3DST_MISS */
    socRxReasonL3SourceMove,       /*  9: CPU_L3SRC_MOVE */
    socRxReasonMcastMiss,          /* 10: CPU_MC_MISS */
    socRxReasonIpMcastMiss,        /* 11: CPU_IPMC_MISS */
    socRxReasonUnknownVlan,        /* 12: CPU_UVLAN */
    socRxReasonL3HeaderError,      /* 13: CPU_L3HDR_ERR */
    socRxReasonProtocol,           /* 14: CPU_PROTOCOL_PKT */
    socRxReasonDosAttack,          /* 15: CPU_DOS_ATTACK */
    socRxReasonMartianAddr,        /* 16: CPU_MARTIAN_ADDR */
    socRxReasonTunnelError,        /* 17: CPU_TUNNEL_ERR */
    socRxReasonL2DestMiss,         /* 18: CPU_DLF */
    socRxReasonIcmpRedirect,       /* 19: ICMP_REDIRECT */
    socRxReasonL3Slowpath,         /* 20: L3_SLOWPATH */
    socRxReasonParityError,        /* 21: PARITY_ERROR */
    socRxReasonL3MtuFail,          /* 22: L3_MTU_CHECK_FAIL */
    socRxReasonHigigHdrError,      /* 23: HGHDR_ERROR */
    socRxReasonMcastIdxError,      /* 24: MCIDX_ERROR */
    socRxReasonL2SourceMiss,       /* 25: CPU_SLF */
    socRxReasonClassBasedMove,     /* 26: CBSM_PREVENTED */
    socRxReasonL3AddrBindFail,     /* 27: MAC_BIND_FAIL */
    socRxReasonMplsLabelMiss,      /* 28: MPLS_LABEL_MISS */
    socRxReasonMplsInvalidAction,  /* 29: MPLS_INVALID_ACTION */
    socRxReasonMplsInvalidPayload, /* 30: MPLS_INVALID_PAYLOAD */
    socRxReasonTunnelObjectValidationFail,  /* 31: TUNNEL_OBJECT_VALIDATION_FAIL */
    socRxReasonMplsSequenceNumber, /* 32: MPLS_SEQ_NUM_FAIL */
    socRxReasonL2NonUnicastMiss,   /* 33: PBT_NONUC_PKT */
    socRxReasonNhop,               /* 34: L3_NEXT_HOP */
    socRxReasonL3Cpu,              /* 35: CPU_L3CPU */
    socRxReasonStation,            /* 36: MY_STATION */
    socRxReasonNivPrioDrop,        /* 37: DOT1P_ADMITTANCE_DISCARD */
    socRxReasonNivRpfFail,         /* 38: RPF_LOOKUP_MISS */
    socRxReasonUnknownSubtendingPort, /* 39: UNKOWN SUBTENDING PORT */
    socRxReasonTunnelAdaptLookupMiss, /* 40: TUNNEL_ADAPT_LOOKUP_MISS_DROP */
    socRxReasonTimeSync,           /* 41: TIME_SYNC */
    socRxReasonPacketFlowSelectMiss,  /* 42: PKT_FLOW_SELECT_MISS */
    socRxReasonTunnelDecapEcnError,/* 43: TUNNEL_DECAP_ECN_ERROR (new) */
    socRxReasonFailoverDrop, /* 44: PROTECTION_DATA_DROP */
    socRxReasonPacketFlowSelect,   /* 45: PKT_FLOW_SELECT */
    socRxReasonOtherLookupMiss,    /* 46: CPU_FORWARDING_OTHER */
    socRxReasonInvalidTpid,        /* 47: INVALID_TPID */
    socRxReasonMplsControlPacket,  /* 48: MPLS_CONTROL_PKT */
    socRxReasonTunnelError,        /* 49: TUNNEL_ERROR */
    socRxReasonTunnelTtlError,     /* 50: TUNNEL_TTL_ERR */
#if 1
    socRxReasonInvalid,            /* 51:NOT DEFINED */
#endif
    socRxReasonCongestionCnm,      /* 52: ICNM */
    socRxReasonMplsIllegalReservedLabel, /* 53: MPLS_ILLEGAL_RESERVED_LABEL */
    socRxReasonMplsRouterAlertLabel, /* 54: MPLS_ALERT_LABEL */
    socRxReasonL3HeaderError,      /* 55: L3_HEADER_ERROR */
    socRxReasonL2HeaderError,      /* 56: L2_HDR_ERROR */
    socRxReasonTtl1,               /* 57: TTL1 */
    socRxReasonTtl,                /* 58: TTL */
    socRxReasonFcoeZoneCheckFail,  /* 59: FCOE_ZONE_CHECK_FAIL */
    socRxReasonNat,                /* 60: NAT_DROP_REASON_ENCODING */
    socRxReasonNat,                /* 61: -> */
    socRxReasonNat,                /* 62: 3-bit */
    socRxReasonIpmcInterfaceMismatch /* 63: CPU_IPMC_INTERFACE_MISMATCH */
};

static soc_rx_reason_t *dcb36_rx_reason_maps[] = {
    dcb36_rx_reason_map,
    dcb26_rx_reason_nat_encoding_map,
    NULL
};

SETFUNCERR(36, tx_l2pbm, pbmp_t)
SETFUNCERR(36, tx_utpbm, pbmp_t)
SETFUNCERR(36, tx_l3pbm, pbmp_t)
SETFUNCERR(36, tx_crc, int)
SETFUNCERR(36, tx_cos, int)
SETFUNCERR(36, tx_destmod, uint32)
SETFUNCERR(36, tx_destport, uint32)
SETFUNCERR(36, tx_opcode, uint32)
SETFUNCERR(36, tx_srcmod, uint32)
SETFUNCERR(36, tx_srcport, uint32)
SETFUNCERR(36, tx_prio, uint32)
SETFUNCERR(36, tx_pfm, uint32)

GETFUNCNULL(36, stat)
SETFUNCNULL2(36, stat)

dcb_op_t dcb36_op = {
    36,
    sizeof(dcb36_t),
    dcb36_rx_reason_maps,
    dcb0_rx_reason_map_get,
    ep_to_cpu_hdr36_rx_reasons_get,
    dcb36_init,
    dcb36_add_tx,
    dcb36_add_rx,
    dcb36_intr_info,
    dcb36_reqcount_set,
    dcb36_reqcount_get,
    dcb36_xfercount_get,
    dcb36_addr_set,
    dcb36_addr_get,
    dcb36_paddr_get,
    dcb36_done_set,
    dcb36_done_get,
    dcb36_sg_set,
    dcb36_sg_get,
    dcb36_chain_set,
    dcb36_chain_get,
    dcb36_reload_set,
    dcb36_reload_get,
    dcb36_desc_intr_set,
    dcb36_desc_intr_get,
    dcb36_tx_l2pbm_set,
    dcb36_tx_utpbm_set,
    dcb36_tx_l3pbm_set,
    dcb36_tx_crc_set,
    dcb36_tx_cos_set,
    dcb36_tx_destmod_set,
    dcb36_tx_destport_set,
    dcb36_tx_opcode_set,
    dcb36_tx_srcmod_set,
    dcb36_tx_srcport_set,
    dcb36_tx_prio_set,
    dcb36_tx_pfm_set,
    ep_to_cpu_hdr36_rx_untagged_get,
    ep_to_cpu_hdr36_rx_crc_get,
    ep_to_cpu_hdr36_rx_cos_get,
    ep_to_cpu_hdr36_rx_destmod_get,
    ep_to_cpu_hdr36_rx_destport_get,
    ep_to_cpu_hdr36_rx_opcode_get,
    ep_to_cpu_hdr36_rx_classtag_get,
    ep_to_cpu_hdr36_rx_matchrule_get,
    dcb36_rx_start_get,
    dcb36_rx_end_get,
    dcb36_rx_error_get,
    ep_to_cpu_hdr36_rx_prio_get,
    ep_to_cpu_hdr36_rx_reason_get,
    ep_to_cpu_hdr36_rx_reason_hi_get,
    ep_to_cpu_hdr36_rx_ingport_get,
    ep_to_cpu_hdr36_rx_srcport_get,
    ep_to_cpu_hdr36_rx_srcmod_get,
    ep_to_cpu_hdr36_rx_mcast_get,
    ep_to_cpu_hdr36_rx_vclabel_get,
    ep_to_cpu_hdr36_rx_mirror_get,
    ep_to_cpu_hdr36_rx_timestamp_get,
    ep_to_cpu_hdr36_rx_timestamp_upper_get,
    dcb36_hg_set,
    dcb36_hg_get,
    dcb36_stat_set,
    dcb36_stat_get,
    dcb36_purge_set,
    dcb36_purge_get,
    ep_to_cpu_hdr36_mhp_get,
    ep_to_cpu_hdr36_outer_vid_get,
    ep_to_cpu_hdr36_outer_pri_get,
    ep_to_cpu_hdr36_outer_cfi_get,
    ep_to_cpu_hdr36_rx_outer_tag_action_get,
    ep_to_cpu_hdr36_inner_vid_get,
    ep_to_cpu_hdr36_inner_pri_get,
    ep_to_cpu_hdr36_inner_cfi_get,
    ep_to_cpu_hdr36_rx_inner_tag_action_get,
    ep_to_cpu_hdr36_rx_bpdu_get,
    ep_to_cpu_hdr36_rx_l3_intf_get,
    ep_to_cpu_hdr36_rx_decap_tunnel_get,
    ep_to_cpu_hdr36_rx_switch_drop_get,
    ep_to_cpu_hdr36_olp_encap_oam_pkt_get,
    dcb36_read_ecc_error_get,
    dcb36_desc_remaining_get,
    dcb36_desc_remaining_set,
    dcb36_desc_status_get,
    dcb36_desc_status_set,
    NULL,
};
#endif /* BCM_TRIDENT3_SUPPORT */

#if defined(BCM_HURRICANE3_SUPPORT)
/*
 * DCB Type 34 Support
 */
GETFUNCEXPR(34, rx_l3_intf, ((d->replicated) ? (d->repl_nhi) :
        (((d->repl_nhi) & 0x7ff) + _SHR_L3_EGRESS_IDX_MIN)))

dcb_op_t dcb34_op = {
    34,
    sizeof(dcb34_t),
    dcb31_rx_reason_maps,
    dcb0_rx_reason_map_get,
    dcb0_rx_reasons_get,
    dcb19_init,
    dcb19_addtx,
    dcb19_addrx,
    dcb19_intrinfo,
    dcb19_reqcount_set,
    dcb19_reqcount_get,
    dcb19_xfercount_get,
    dcb0_addr_set,
    dcb0_addr_get,
    dcb0_paddr_get,
    dcb19_done_set,
    dcb19_done_get,
    dcb19_sg_set,
    dcb19_sg_get,
    dcb19_chain_set,
    dcb19_chain_get,
    dcb19_reload_set,
    dcb19_reload_get,
    dcb19_desc_intr_set,
    dcb19_desc_intr_get,
    dcb19_tx_l2pbm_set,
    dcb19_tx_utpbm_set,
    dcb19_tx_l3pbm_set,
    dcb19_tx_crc_set,
    dcb19_tx_cos_set,
    dcb19_tx_destmod_set,
    dcb19_tx_destport_set,
    dcb19_tx_opcode_set,
    dcb19_tx_srcmod_set,
    dcb19_tx_srcport_set,
    dcb19_tx_prio_set,
    dcb19_tx_pfm_set,
    dcb31_rx_untagged_get,
    dcb19_rx_crc_get,
    dcb31_rx_cos_get,
    dcb23_rx_destmod_get,
    dcb23_rx_destport_get,
    dcb23_rx_opcode_get,
    dcb23_rx_classtag_get,
    dcb23_rx_matchrule_get,
    dcb19_rx_start_get,
    dcb19_rx_end_get,
    dcb19_rx_error_get,
    dcb23_rx_prio_get,
    dcb23_rx_reason_get,
    dcb23_rx_reason_hi_get,
    dcb23_rx_ingport_get,
    dcb23_rx_srcport_get,
    dcb23_rx_srcmod_get,
    dcb23_rx_mcast_get,
    dcb23_rx_vclabel_get,
    dcb31_rx_mirror_get,
    dcb23_rx_timestamp_get,
    dcb31_rx_timestamp_upper_get,
    dcb19_hg_set,
    dcb19_hg_get,
    dcb19_stat_set,
    dcb19_stat_get,
    dcb19_purge_set,
    dcb19_purge_get,
    dcb23_mhp_get,
    dcb31_outer_vid_get,
    dcb31_outer_pri_get,
    dcb31_outer_cfi_get,
    dcb31_rx_outer_tag_action_get,
    dcb31_inner_vid_get,
    dcb31_inner_pri_get,
    dcb31_inner_cfi_get,
    dcb31_rx_inner_tag_action_get,
    dcb31_rx_bpdu_get,
    dcb34_rx_l3_intf_get,
    dcb19_rx_decap_tunnel_get,
    dcb31_rx_switch_drop_get,
    dcb23_olp_encap_oam_pkt_get,
    dcb23_read_ecc_error_get,
    dcb23_desc_remaining_get,
    dcb23_desc_remaining_set,
    dcb23_desc_status_get,
    dcb23_desc_status_set,
    NULL,
};
#endif /* BCM_HURRICANE3_SUPPORT */

#if defined(BCM_GREYHOUND2_SUPPORT)
/*
 * DCB Type 37 Support
 */
/* From FORMAT CPU_OPCODES */
static soc_rx_reason_t
dcb37_rx_reason_map[] = {
    socRxReasonUnknownVlan,        /* Offset 0 */
    socRxReasonL2SourceMiss,       /* Offset 1 */
    socRxReasonL2DestMiss,         /* Offset 2 */
    socRxReasonL2Move,             /* Offset 3 */
    socRxReasonL2Cpu,              /* Offset 4 */
    socRxReasonSampleSource,       /* Offset 5 */
    socRxReasonSampleDest,         /* Offset 6 */
    socRxReasonL3SourceMiss,       /* Offset 7 */
    socRxReasonL3DestMiss,         /* Offset 8 */
    socRxReasonL3SourceMove,       /* Offset 9 */
    socRxReasonMcastMiss,          /* Offset 10 */
    socRxReasonIpMcastMiss,        /* Offset 11 */
    socRxReasonFilterMatch,        /* Offset 12 */
    socRxReasonL3HeaderError,      /* Offset 13 */
    socRxReasonProtocol,           /* Offset 14 */
    socRxReasonDosAttack,          /* Offset 15 */
    socRxReasonMartianAddr,        /* Offset 16 */
    socRxReasonTunnelError,        /* Offset 17 */
    socRxReasonL2MtuFail,          /* Offset 18 */
    socRxReasonIcmpRedirect,       /* Offset 19 */
    socRxReasonL3Slowpath,         /* Offset 20 */
    socRxReasonParityError,        /* Offset 21 */
    socRxReasonL3MtuFail,          /* Offset 22 */
    socRxReasonHigigHdrError,      /* Offset 23 */
    socRxReasonMcastIdxError,      /* Offset 24 */
    socRxReasonVlanFilterMatch,    /* Offset 25 */
    socRxReasonClassBasedMove,     /* Offset 26 */
    socRxReasonL2LearnLimit,       /* Offset 27 */
    socRxReasonMplsLabelMiss,      /* Offset 28 */
    socRxReasonMplsInvalidAction,  /* Offset 29 */
    socRxReasonMplsInvalidPayload, /* Offset 30 */
    socRxReasonMplsTtl,            /* Offset 31 */
    socRxReasonMplsSequenceNumber, /* Offset 32 */
    socRxReasonL2NonUnicastMiss,   /* Offset 33 */
    socRxReasonNhop,               /* Offset 34 */
    socRxReasonMplsCtrlWordError,  /* Offset 35 */
    socRxReasonTimeSync,           /* Offset 36 */
    socRxReasonOAMSlowpath,        /* Offset 37 */
    socRxReasonOAMError,           /* Offset 38 */
    socRxReasonOAMLMDM,            /* Offset 39 */
    socRxReasonL3AddrBindFail,     /* Offset 40 */
    socRxReasonVlanTranslate,      /* Offset 41 */
    socRxReasonNiv,                /* Offset 42 */
    socRxReasonNiv,                /* Offset 43 */
    socRxReasonNiv,                /* Offset 44 */
    socRxReasonStation,            /* Offset 45 */
    socRxReasonL2StuFail,          /* Offset 46 */
    socReasonSrCounterExceeded,    /* Offset 47 */
    socRxReasonSrCopyToCpuBit0,    /* Offset 48 */
    socRxReasonSrCopyToCpuBit1,    /* Offset 49 */
    socRxReasonSrCopyToCpuBit2,    /* Offset 50 */
    socRxReasonSrCopyToCpuBit3,    /* Offset 51 */
    socRxReasonSrCopyToCpuBit4,    /* Offset 52 */
    socRxReasonSrCopyToCpuBit5,    /* Offset 53 */
    socRxReasonInvalid,            /* Offset 54 */
    socRxReasonInvalid,            /* Offset 55 */
    socRxReasonInvalid,            /* Offset 56 */
    socRxReasonInvalid,            /* Offset 57 */
    socRxReasonInvalid,            /* Offset 58 */
    socRxReasonInvalid,            /* Offset 59 */
    socRxReasonInvalid,            /* Offset 60 */
    socRxReasonInvalid,            /* Offset 61 */
    socRxReasonInvalid,            /* Offset 62 */
    socRxReasonInvalid             /* Offset 63 */
};
static soc_rx_reason_t *dcb37_rx_reason_maps[] = {
    dcb37_rx_reason_map,
    NULL
};

GETFUNCEXPR(37, rx_l3_intf, ((d->replicated) ? (d->repl_nhi) :
        (((d->repl_nhi) & 0x7ff) + _SHR_L3_EGRESS_IDX_MIN)))
GETFUNCNULL(37, read_ecc_error)
GETFUNCNULL(37, desc_remaining)
SETFUNCNULL(37, desc_remaining)
GETFUNCNULL(37, desc_status)
SETFUNCNULL(37, desc_status)

dcb_op_t dcb37_op = {
    37,
    sizeof(dcb37_t),
    dcb37_rx_reason_maps,
    dcb0_rx_reason_map_get,
    dcb0_rx_reasons_get,
    dcb19_init,
    dcb19_addtx,
    dcb19_addrx,
    dcb19_intrinfo,
    dcb19_reqcount_set,
    dcb19_reqcount_get,
    dcb19_xfercount_get,
    dcb0_addr_set,
    dcb0_addr_get,
    dcb0_paddr_get,
    dcb19_done_set,
    dcb19_done_get,
    dcb19_sg_set,
    dcb19_sg_get,
    dcb19_chain_set,
    dcb19_chain_get,
    dcb19_reload_set,
    dcb19_reload_get,
    dcb19_desc_intr_set,
    dcb19_desc_intr_get,
    dcb19_tx_l2pbm_set,
    dcb19_tx_utpbm_set,
    dcb19_tx_l3pbm_set,
    dcb19_tx_crc_set,
    dcb19_tx_cos_set,
    dcb19_tx_destmod_set,
    dcb19_tx_destport_set,
    dcb19_tx_opcode_set,
    dcb19_tx_srcmod_set,
    dcb19_tx_srcport_set,
    dcb19_tx_prio_set,
    dcb19_tx_pfm_set,
    dcb31_rx_untagged_get,
    dcb19_rx_crc_get,
    dcb31_rx_cos_get,
    dcb23_rx_destmod_get,
    dcb23_rx_destport_get,
    dcb23_rx_opcode_get,
    dcb23_rx_classtag_get,
    dcb23_rx_matchrule_get,
    dcb19_rx_start_get,
    dcb19_rx_end_get,
    dcb19_rx_error_get,
    dcb23_rx_prio_get,
    dcb23_rx_reason_get,
    dcb23_rx_reason_hi_get,
    dcb23_rx_ingport_get,
    dcb23_rx_srcport_get,
    dcb23_rx_srcmod_get,
    dcb23_rx_mcast_get,
    dcb23_rx_vclabel_get,
    dcb31_rx_mirror_get,
    dcb23_rx_timestamp_get,
    dcb31_rx_timestamp_upper_get,
    dcb19_hg_set,
    dcb19_hg_get,
    dcb19_stat_set,
    dcb19_stat_get,
    dcb19_purge_set,
    dcb19_purge_get,
    dcb23_mhp_get,
    dcb31_outer_vid_get,
    dcb31_outer_pri_get,
    dcb31_outer_cfi_get,
    dcb31_rx_outer_tag_action_get,
    dcb31_inner_vid_get,
    dcb31_inner_pri_get,
    dcb31_inner_cfi_get,
    dcb31_rx_inner_tag_action_get,
    dcb31_rx_bpdu_get,
    dcb37_rx_l3_intf_get,
    dcb19_rx_decap_tunnel_get,
    dcb31_rx_switch_drop_get,
    dcb23_olp_encap_oam_pkt_get,
    dcb37_read_ecc_error_get,
    dcb37_desc_remaining_get,
    dcb37_desc_remaining_set,
    dcb37_desc_status_get,
    dcb37_desc_status_set,
    NULL,
};
#endif /* BCM_GREYHOUND2_SUPPORT */

#if defined(BCM_TOMAHAWK3_SUPPORT)
SETFUNCFIELD(38, reqcount, c_count, uint32 count, count)
GETFUNCFIELD(38, reqcount, c_count)
GETFUNCFIELD(38, xfercount, count)
SETFUNCFIELD(38, done, done, int val, val ? 1 : 0)
GETFUNCFIELD(38, done, done)
SETFUNCFIELD(38, sg, c_sg, int val, val ? 1 : 0)
GETFUNCFIELD(38, sg, c_sg)
SETFUNCFIELD(38, chain, c_chain, int val, val ? 1 : 0)
GETFUNCFIELD(38, chain, c_chain)
SETFUNCFIELD(38, reload, c_reload, int val, val ? 1 : 0)
GETFUNCFIELD(38, reload, c_reload)
GETFUNCFIELD(38, rx_start, start)
GETFUNCFIELD(38, rx_end, end)
GETFUNCFIELD(38, rx_error, error)
SETFUNCFIELD(38, hg, c_hg, uint32 hg, hg)
GETFUNCFIELD(38, hg, c_hg)
SETFUNCFIELD(38, purge, c_purge, uint32 purge, purge)
GETFUNCFIELD(38, purge, c_purge)
GETFUNCFIELD(38, read_ecc_error, ecc_error)
GETFUNCFIELD(38, desc_status, c_desc_status)
GETFUNCFIELD(38, desc_intr, c_intr)
GETFUNCFIELD(38, desc_remaining, c_desc)

GETHDRFUNCFIELD(38, rx_timestamp, timestamp)
GETHDRFUNCFIELD(38, rx_timestamp_upper, timestamp_hi)
GETHDRPTREXPR(38, mhp, &(h->mh0))
GETHDRFUNCFIELD(38, outer_vid, outer_vid)
GETHDRFUNCFIELD(38, outer_pri, outer_pri)
GETHDRFUNCFIELD(38, outer_cfi, outer_cfi)
GETHDRFUNCFIELD(38, rx_outer_tag_action, word3.overlay1.otag_action)
GETHDRFUNCNULL(38, inner_vid)
GETHDRFUNCNULL(38, inner_pri)
GETHDRFUNCNULL(38, inner_cfi)
GETHDRFUNCNULL(38, rx_inner_tag_action)
GETHDRFUNCFIELD(38, rx_bpdu, word0.overlay1.bpdu)

static uint32 ep_to_cpu_hdr38_rx_crc_get(void *hdr) {
    return 0;
}

static uint32
ep_to_cpu_hdr38_rx_untagged_get(void *hdr, int dt_mode, int ingport_is_hg)
{
    ep_to_cpu_hdr38_t *h = (ep_to_cpu_hdr38_t *)hdr;
    uint32 tag_status = h->word3.overlay1.tag_status;

    COMPILER_REFERENCE(dt_mode);
    /* We have a case where though the number of bits for the
     * tag status is 2, we support only 2 states
     * 0 = untagged
     * 1 = single outer-tag
     */
    return (ingport_is_hg ?
            ((tag_status) ? 0 : 2) :
            ((tag_status) ? 2 : 3));
}

static void
ep_to_cpu_hdr38_rx_reasons_get(dcb_op_t *dcb_op, void *hdr, soc_rx_reasons_t *reasons)
{
    soc_rx_reason_t *map, *encoding_map;
    uint32 opcode[2], encoding[3];
    uint32 bit_val;
    int word_idx, bit_idx, opcode_idx, map_idx, word_count;
    int enc_bit = 0;

    SOC_RX_REASON_CLEAR_ALL(*reasons);

    opcode[0] = dcb_op->rx_reason_get(hdr);
    opcode[1] = dcb_op->rx_reason_hi_get(hdr);
    word_count = 2;
    sal_memset(encoding, 0, sizeof(encoding));
    map = dcb_op->rx_reason_maps[0];
    for (word_idx = 0; word_idx < word_count; word_idx++) {
        for (bit_idx = 0; bit_idx < 32; bit_idx++) {
            opcode_idx = word_idx * 32 + bit_idx;
            bit_val = opcode[word_idx] & (1 << bit_idx) ? 1 : 0;
            for (map_idx = 0; ; map_idx++) {
                encoding_map = dcb_op->rx_reason_maps[1 + map_idx];
                if (encoding_map == NULL) {
                    break;
                }
                if (map[opcode_idx] != encoding_map[0]) {
                    continue;
                }
                if (opcode_idx == 0 || map[opcode_idx - 1] != encoding_map[0]) {
                    enc_bit = 0;
                }
                encoding[map_idx] |= bit_val << enc_bit;
                enc_bit++;
                break;
            }
            if (encoding_map == NULL && bit_val) {
                SOC_RX_REASON_SET(*reasons, map[opcode_idx]);
            }
        }
    }

    for (map_idx = 0; ; map_idx++) {
        encoding_map = dcb_op->rx_reason_maps[1 + map_idx];
        if (encoding_map == NULL) {
            break;
        }
        if (encoding[map_idx] != socRxReasonInvalid) {
            SOC_RX_REASON_SET(*reasons, encoding_map[encoding[map_idx]]);
        }
    }

    /* BPDU bit should be a reason, paste it in here */
    if (dcb_op->rx_bpdu_get(hdr)) {
        SOC_RX_REASON_SET(*reasons, socRxReasonBpdu);
    }

    return;
}

GETHDRFUNCFIELD(38, rx_cos, word0.overlay2.cpu_cos)
GETHDRHG2FUNCFIELD(38, rx_destmod, dst_mod)
GETHDRHG2FUNCFIELD(38, rx_destport, dst_port)

GETHDRHG2FUNCFIELD(38, rx_opcode, opcode)
GETHDRHG2FUNCEXPR(38, rx_classtag, (h->ppd_overlay1.ppd_type != 1 ? 0 :
                                 (h->ppd_overlay2.ctag_hi << 8) |
                                 (h->ppd_overlay2.ctag_lo)))
GETHDRFUNCFIELD(38, rx_matchrule, match_rule)
GETHDRHG2FUNCFIELD(38, rx_prio, vlan_pri) /* outer_pri */

static uint32
ep_to_cpu_hdr38_rx_reason_get(void *hdr)
{
    ep_to_cpu_hdr38_t *h = (ep_to_cpu_hdr38_t *)hdr;

    return (h->switch_pkt) ? (h->reason) : 0;
}

static uint32
ep_to_cpu_hdr38_rx_reason_hi_get(void *hdr)
{
    ep_to_cpu_hdr38_t *h = (ep_to_cpu_hdr38_t *)hdr;

    return (h->switch_pkt) ? (h->reason_hi) : 0;
}

GETHDRFUNCFIELD(38, rx_ingport, srcport)
GETHDRHG2FUNCFIELD(38, rx_srcport, src_port)
GETHDRHG2FUNCFIELD(38, rx_srcmod, src_mod)
GETHDRHG2FUNCEXPR(38, rx_mcast, ((h->ppd_overlay1.dst_mod << 8) |
                              (h->ppd_overlay1.dst_port)))
GETHDRHG2FUNCEXPR(38, rx_vclabel, ((h->ppd_overlay1.vc_label_19_16 << 16) |
                              (h->ppd_overlay1.vc_label_15_8 << 8) |
                              (h->ppd_overlay1.vc_label_7_0)))
GETHDRFUNCEXPR(38, rx_mirror, ((SOC_CPU_OPCODE_TYPE_IP_0 ==
                          h->word3.overlay1.cpu_opcode_type) ?
                          (h->reason & (1 << 17)) : 0))

static _shr_rx_decap_tunnel_t dcb38_rx_decap_tunnel_map[] = {
    _SHR_RX_DECAP_NONE,
    _SHR_RX_DECAP_IP,
    _SHR_RX_DECAP_L2MPLS_1LABEL,
    _SHR_RX_DECAP_L2MPLS_2LABEL,
    _SHR_RX_DECAP_MPLS_3LABEL,
    _SHR_RX_DECAP_L3MPLS_1LABEL,
    _SHR_RX_DECAP_L3MPLS_2LABEL,
    _SHR_RX_DECAP_MPLS_3LABEL
};

static uint32 ep_to_cpu_hdr38_rx_decap_tunnel_get(void *hdr) {
    ep_to_cpu_hdr38_t *h = (ep_to_cpu_hdr38_t *)hdr;
    /* Check for array bounds */
    if ( (h->decap_tunnel_type <
    (sizeof (dcb38_rx_decap_tunnel_map) / sizeof (_shr_rx_decap_tunnel_t))) ) {
        return (uint32)dcb38_rx_decap_tunnel_map[h->decap_tunnel_type];
    }
    return (uint32)_SHR_RX_DECAP_NONE;
}
GETHDRFUNCFIELD(38, rx_l3_intf, word3.overlay1.ing_l3_intf)
GETHDRFUNCNULL(38, rx_switch_drop)
GETHDRFUNCNULL(38, olp_encap_oam_pkt)

static void
dcb38_init(dcb_t *dcb)
{
    uint32      *d = (uint32 *)dcb;

    d[0] = d[1] = d[2] = d[3] = 0;
}

static int
dcb38_add_rx(dv_t *dv, sal_vaddr_t addr, uint32 count, uint32 flags)
{
    dcb38_t           *d;     /* DCB */
    uint32            *di;    /* DCB integer pointer */
    sal_paddr_t        paddr;

    if(!addr)
        return SOC_E_PARAM;

    d = (dcb38_t *)SOC_DCB_IDX2PTR(dv->dv_unit, dv->dv_dcb, dv->dv_vcnt);

    if (dv->dv_vcnt > 0) {      /* chain off previous dcb */
        d[-1].c_chain = 1;
    }

    di = (uint32 *)d;
    di[0] = di[1] = di[2] = di[3] = 0;

    if (addr) {
        paddr = soc_cm_l2p(dv->dv_unit, (void *)addr);
        d->addr_lo = PTR_TO_INT(paddr);
        d->addr_hi = (0x10000000 | PTR_HI_TO_INT(paddr));
    }

    d->c_count = count;
    d->c_sg = 1;

    dv->dv_vcnt += 1;
    return dv->dv_cnt - dv->dv_vcnt;
}

static int
dcb38_add_tx(dv_t *dv, sal_vaddr_t addr, uint32 count,
            pbmp_t l2pbm, pbmp_t utpbm, pbmp_t l3pbm, uint32 flags, uint32 *hgh)
{
    dcb38_t           *d;     /* DCB */
    uint32            *di;    /* DCB integer pointer */
    sal_paddr_t       paddr;  /* Packet buffer physical address */

    if(!addr)
        return SOC_E_PARAM;

    d = (dcb38_t *)SOC_DCB_IDX2PTR(dv->dv_unit, dv->dv_dcb, dv->dv_vcnt);

    if (addr)
        paddr = soc_cm_l2p(dv->dv_unit, (void *)addr);

    if (dv->dv_vcnt > 0 && (dv->dv_flags & DV_F_COMBINE_DCB) &&
        (d[-1].c_sg != 0) &&
        ((d[-1].addr_lo + d[-1].c_count) == paddr) &&
        (d[-1].c_count + count <= DCB_MAX_REQCOUNT)) {
        d[-1].c_count += count;
        return dv->dv_cnt - dv->dv_vcnt;
    }

    if (dv->dv_vcnt >= dv->dv_cnt) {
        return SOC_E_FULL;
    }
    if (dv->dv_vcnt > 0) {      /* chain off previous dcb */
        d[-1].c_chain = 1;
    }

    di = (uint32 *)d;
    di[0] = di[1] = di[2] = di[3] = 0;

    d->addr_lo = PTR_TO_INT(paddr);
    d->addr_hi = (0x10000000 | PTR_HI_TO_INT(paddr));
    d->c_count = count;
    d->c_sg = 1;

    d->c_purge = SOC_DMA_PURGE_GET(flags);

    if (SOC_DMA_HG_GET(flags)) {
        d->c_hg = 1;
    }

    dv->dv_vcnt += 1;

    return dv->dv_cnt - dv->dv_vcnt;
}

static uint32
dcb38_intr_info(int unit, dcb_t *dcb, int tx, uint32 *count)
{
    dcb38_t *d = (dcb38_t *)dcb;     /*  DCB */
    uint32  f;                             /* SOC_DCB_INFO_* flags */

    if (!d->done) {
        return 0;
    }

    f = SOC_DCB_INFO_DONE;
    if (tx) {
        if (!d->c_sg) {
            f |= SOC_DCB_INFO_PKTEND;
        }
    } else {
        if (d->end) {
            f |= SOC_DCB_INFO_PKTEND;
        }
    }

    *count = d->count;
    return f;
}

/* the addr related functions are the same for all dcb types */
static void
dcb38_addr_set(int unit, dcb_t *dcb, sal_vaddr_t addr)
{
    dcb38_t      *d = (dcb38_t *)dcb;     /*  DCB */
    sal_paddr_t  paddr;

    if (addr == 0) {
        d->addr_lo = 0;
        d->addr_hi = 0;
    } else {
        paddr = soc_cm_l2p(unit, (void *)addr);
        d->addr_lo = PTR_TO_INT(paddr);
        d->addr_hi = (0x10000000 | PTR_HI_TO_INT(paddr));
    }
}

static sal_vaddr_t
dcb38_addr_get(int unit, dcb_t *dcb)
{
    dcb38_t      *d = (dcb38_t *)dcb;     /*  DCB */
    sal_paddr_t        paddr;

#if defined(COMPILER_OVERRIDE_NO_LONGLONG) || defined(__PEDANTIC__)
    paddr = d->addr_lo;
#else
    paddr = (((uint64)(d->addr_hi & ~(0x10000000)) << 32) | d->addr_lo);
#endif

    return (sal_vaddr_t)soc_cm_p2l(unit, paddr);
}

static sal_paddr_t
dcb38_paddr_get(dcb_t *dcb)
{
    uint32      *d = (uint32 *)dcb;

    return (sal_paddr_t)*d;
}

static void
dcb38_desc_intr_set(dcb_t * dcb, int val)
{
    dcb38_t * d = (dcb38_t *)dcb;

    d->c_intr = val;
    return;
}

static uint32
dcb38_desc_status_set(dcb_t * dcb, uint32 val)
{
    dcb38_t * d = (dcb38_t *)dcb;

    d->c_desc_status = val;
    return 0;
}

static uint32
dcb38_desc_remaining_set(dcb_t * dcb, uint32 val)
{
    dcb38_t * d = (dcb38_t *)dcb;

    d->c_desc = val;
    return 0;
}

/*
 * DCB Type 38 Support
 */
/* From FORMAT CPU_OPCODES */
static soc_rx_reason_t
dcb38_rx_reason_map[] = {
    socRxReasonUnknownVlan,        /*  0: CPU_UVLAN */
    socRxReasonL2SourceMiss,       /*  1: CPU_SLF */
    socRxReasonL2DestMiss,         /*  2: CPU_DLF */
    socRxReasonL2Move,             /*  3: CPU_L2MOVE */
    socRxReasonL2Cpu,              /*  4: CPU_L2CPU */
    socRxReasonL3SourceMiss,       /*  5: CPU_L3SRC_MISS */
    socRxReasonL3DestMiss,         /*  6: CPU_L3DST_MISS */
    socRxReasonL3SourceMove,       /*  7: CPU_L3SRC_MOVE */
    socRxReasonMcastMiss,          /*  8: CPU_MC_MISS */
    socRxReasonIpMcastMiss,        /*  9: CPU_IPMC_MISS */
    socRxReasonFilterMatch,        /* 10: CPU_FFP */
    socRxReasonL3HeaderError,      /* 11: CPU_L3HDR_ERR */
    socRxReasonProtocol,           /* 12: CPU_PROTOCOL_PKT */
    socRxReasonDosAttack,          /* 13: CPU_DOS_ATTACK */
    socRxReasonMartianAddr,        /* 14: CPU_MARTIAN_ADDR */
    socRxReasonTunnelError,        /* 15: CPU_TUNNEL_ERR */
    socRxReasonSampleSourceFlex,   /* 16: CPU_SFLOW_FLEX */
    socRxReasonSampleSourceFlex,   /* 17: CPU_SFLOW_FLEX */
    socRxReasonIcmpRedirect,       /* 18: ICMP_REDIRECT */
    socRxReasonL3Slowpath,         /* 19: L3_SLOWPATH */
    socRxReasonParityError,        /* 20: PARITY_ERROR */
    socRxReasonL3MtuFail,          /* 21: L3_MTU_CHECK_FAIL */
    socRxReasonMcastIdxError,      /* 22: MCIDX_ERROR */
    socRxReasonVlanFilterMatch,    /* 23: VFP */
    socRxReasonMplsError,          /* 24: MPLS_LABEL_MISS */
    socRxReasonMplsError,          /* 25: MPLS_INVALID_ACTION */
    socRxReasonMplsError,          /* 26: MPLS_INVALID_PAYLOAD */
    socRxReasonL2NonUnicastMiss,   /* 27: PBT_NONUC_PKT */
    socRxReasonNhop,               /* 28: L3_NEXT_HOP */
    socRxReasonStation,            /* 29: MY_STATION */
    socRxReasonTimeSync,           /* 30: TIME_SYNC */
    socRxReasonTunnelDecapEcnError,/* 31: TUNNEL_DECAP_ECN_ERROR (new) */
    socRxReasonBfdSlowpath,        /* 32: BFD SLOWPATH */
    socRxReasonBfd,                /* 33: BFD_ERROR */
#if 1
    socRxReasonInvalid,            /* 34:NOT DEFINED */
#endif
    socRxReasonMplsUnknownAch,     /* 35: MPLS_UNKNOWN_ACH_ERROR */
    socRxReasonMplsRouterAlertLabel, /* 36: MPLS_ALERT_LABEL */
    socRxReasonIpmcInterfaceMismatch, /* 37: CPU_IPMC_INTERFACE_MISMATCH */
    socRxReasonDlbMonitor,         /* 38: DLB_MONITOR */
    socRxReasonIntTurnAround,      /* 39: INBAND_TELEMETRY */
    socRxReasonEtrapMonitor,       /* 40: ETRAP_MONITOR */
    socRxReasonInvalid,            /* 41: Reserved */
    socRxReasonInvalid,            /* 42: Reserved */
    socRxReasonInvalid,            /* 43: Reserved */
    socRxReasonInvalid,            /* 44: Reserved */
    socRxReasonInvalid,            /* 45: Reserved */
    socRxReasonInvalid,            /* 46: Reserved */
    socRxReasonInvalid,            /* 47: Reserved */
    socRxReasonInvalid,            /* 48: Reserved */
    socRxReasonInvalid,            /* 49: Reserved */
    socRxReasonInvalid,            /* 50: Reserved */
    socRxReasonInvalid,            /* 51: Reserved */
    socRxReasonInvalid,            /* 52: Reserved */
    socRxReasonInvalid,            /* 53: Reserved */
    socRxReasonInvalid,            /* 54: Reserved */
    socRxReasonInvalid,            /* 55: Reserved */
    socRxReasonInvalid,            /* 56: Reserved */
    socRxReasonInvalid,            /* 57: Reserved */
    socRxReasonInvalid,            /* 58: Reserved */
    socRxReasonInvalid,            /* 59: Reserved */
    socRxReasonInvalid,            /* 60: Reserved */
    socRxReasonInvalid,            /* 61: Reserved */
    socRxReasonInvalid,            /* 62: Reserved */
    socRxReasonInvalid             /* 63: Reserved */
};

/* From CPU_SFLOW encoding from TH3 regsfile */
static soc_rx_reason_t dcb38_rx_reason_cpu_sflow_encoding_map[] = {
    socRxReasonSampleSourceFlex,   /* 0: NO_ERRORS
                                    * Basefield, must match the entries above */
    socRxReasonSampleSourceFlex,   /* 1: CPU_SFLOW_FLEX */
    socRxReasonSampleDest,         /* 2: CPU_SFLOW_DST */
    socRxReasonSampleSource,       /* 3: CPU_SFLOW_SRC */
    socRxReasonInvalid,            /* Offset 4 */
    socRxReasonInvalid,            /* Offset 5 */
    socRxReasonInvalid,            /* Offset 6 */
    socRxReasonInvalid,            /* Offset 7 */
    socRxReasonInvalid,            /* Offset 8 */
    socRxReasonInvalid,            /* Offset 9 */
    socRxReasonInvalid,            /* Offset 10 */
    socRxReasonInvalid,            /* Offset 11 */
    socRxReasonInvalid,            /* Offset 12 */
    socRxReasonInvalid,            /* Offset 13 */
    socRxReasonInvalid,            /* Offset 14 */
    socRxReasonInvalid,            /* Offset 15 */
    socRxReasonInvalid,            /* Offset 16 */
    socRxReasonInvalid,            /* Offset 17 */
    socRxReasonInvalid,            /* Offset 18 */
    socRxReasonInvalid,            /* Offset 19 */
    socRxReasonInvalid,            /* Offset 20 */
    socRxReasonInvalid,            /* Offset 21 */
    socRxReasonInvalid,            /* Offset 22 */
    socRxReasonInvalid,            /* Offset 23 */
    socRxReasonInvalid,            /* Offset 24 */
    socRxReasonInvalid,            /* Offset 25 */
    socRxReasonInvalid,            /* Offset 26 */
    socRxReasonInvalid,            /* Offset 27 */
    socRxReasonInvalid,            /* Offset 28 */
    socRxReasonInvalid,            /* Offset 29 */
    socRxReasonInvalid,            /* Offset 30 */
    socRxReasonInvalid,            /* Offset 31 */
    socRxReasonInvalid,            /* Offset 32 */
    socRxReasonInvalid,            /* Offset 33 */
    socRxReasonInvalid,            /* Offset 34 */
    socRxReasonInvalid,            /* Offset 35 */
    socRxReasonInvalid,            /* Offset 36 */
    socRxReasonInvalid,            /* Offset 37 */
    socRxReasonInvalid,            /* Offset 38 */
    socRxReasonInvalid,            /* Offset 39 */
    socRxReasonInvalid,            /* Offset 40 */
    socRxReasonInvalid,            /* Offset 41 */
    socRxReasonInvalid,            /* Offset 42 */
    socRxReasonInvalid,            /* Offset 43 */
    socRxReasonInvalid,            /* Offset 44 */
    socRxReasonInvalid,            /* Offset 45 */
    socRxReasonInvalid,            /* Offset 46 */
    socRxReasonInvalid,            /* Offset 47 */
    socRxReasonInvalid,            /* Offset 48 */
    socRxReasonInvalid,            /* Offset 49 */
    socRxReasonInvalid,            /* Offset 50 */
    socRxReasonInvalid,            /* Offset 51 */
    socRxReasonInvalid,            /* Offset 52 */
    socRxReasonInvalid,            /* Offset 53 */
    socRxReasonInvalid,            /* Offset 54 */
    socRxReasonInvalid,            /* Offset 55 */
    socRxReasonInvalid,            /* Offset 56 */
    socRxReasonInvalid,            /* Offset 57 */
    socRxReasonInvalid,            /* Offset 58 */
    socRxReasonInvalid,            /* Offset 59 */
    socRxReasonInvalid,            /* Offset 60 */
    socRxReasonInvalid,            /* Offset 61 */
    socRxReasonInvalid,            /* Offset 62 */
    socRxReasonInvalid             /* Offset 63 */
};

/* From MPLS_PROC_ERROR encoding in TH3 regsfile */
static soc_rx_reason_t dcb38_rx_reason_mpls_proc_err_encoding_map[] = {
    socRxReasonMplsError,          /* 0: NO_ERRORS
                                    * Basefield, must match the entries above */
    socRxReasonMplsInvalidPayload, /* 1: Invalid Payload */
    socRxReasonMplsInvalidAction,  /* 2: Invalid Action */
    socRxReasonMplsLabelMiss,      /* 3: Lookup Label Miss */
    socRxReasonMplsTtl,            /* 4: TTL Check Fail */
    socRxReasonInvalid,            /* Offset 5 */
    socRxReasonInvalid,            /* Offset 6 */
    socRxReasonInvalid,            /* Offset 7 */
    socRxReasonInvalid,            /* Offset 8 */
    socRxReasonInvalid,            /* Offset 9 */
    socRxReasonInvalid,            /* Offset 10 */
    socRxReasonInvalid,            /* Offset 11 */
    socRxReasonInvalid,            /* Offset 12 */
    socRxReasonInvalid,            /* Offset 13 */
    socRxReasonInvalid,            /* Offset 14 */
    socRxReasonInvalid,            /* Offset 15 */
    socRxReasonInvalid,            /* Offset 16 */
    socRxReasonInvalid,            /* Offset 17 */
    socRxReasonInvalid,            /* Offset 18 */
    socRxReasonInvalid,            /* Offset 19 */
    socRxReasonInvalid,            /* Offset 20 */
    socRxReasonInvalid,            /* Offset 21 */
    socRxReasonInvalid,            /* Offset 22 */
    socRxReasonInvalid,            /* Offset 23 */
    socRxReasonInvalid,            /* Offset 24 */
    socRxReasonInvalid,            /* Offset 25 */
    socRxReasonInvalid,            /* Offset 26 */
    socRxReasonInvalid,            /* Offset 27 */
    socRxReasonInvalid,            /* Offset 28 */
    socRxReasonInvalid,            /* Offset 29 */
    socRxReasonInvalid,            /* Offset 30 */
    socRxReasonInvalid,            /* Offset 31 */
    socRxReasonInvalid,            /* Offset 32 */
    socRxReasonInvalid,            /* Offset 33 */
    socRxReasonInvalid,            /* Offset 34 */
    socRxReasonInvalid,            /* Offset 35 */
    socRxReasonInvalid,            /* Offset 36 */
    socRxReasonInvalid,            /* Offset 37 */
    socRxReasonInvalid,            /* Offset 38 */
    socRxReasonInvalid,            /* Offset 39 */
    socRxReasonInvalid,            /* Offset 40 */
    socRxReasonInvalid,            /* Offset 41 */
    socRxReasonInvalid,            /* Offset 42 */
    socRxReasonInvalid,            /* Offset 43 */
    socRxReasonInvalid,            /* Offset 44 */
    socRxReasonInvalid,            /* Offset 45 */
    socRxReasonInvalid,            /* Offset 46 */
    socRxReasonInvalid,            /* Offset 47 */
    socRxReasonInvalid,            /* Offset 48 */
    socRxReasonInvalid,            /* Offset 49 */
    socRxReasonInvalid,            /* Offset 50 */
    socRxReasonInvalid,            /* Offset 51 */
    socRxReasonInvalid,            /* Offset 52 */
    socRxReasonInvalid,            /* Offset 53 */
    socRxReasonInvalid,            /* Offset 54 */
    socRxReasonInvalid,            /* Offset 55 */
    socRxReasonInvalid,            /* Offset 56 */
    socRxReasonInvalid,            /* Offset 57 */
    socRxReasonInvalid,            /* Offset 58 */
    socRxReasonInvalid,            /* Offset 59 */
    socRxReasonInvalid,            /* Offset 60 */
    socRxReasonInvalid,            /* Offset 61 */
    socRxReasonInvalid,            /* Offset 62 */
    socRxReasonInvalid             /* Offset 63 */
};

static soc_rx_reason_t *dcb38_rx_reason_maps[] = {
    dcb38_rx_reason_map,
    dcb38_rx_reason_cpu_sflow_encoding_map,
    dcb38_rx_reason_mpls_proc_err_encoding_map,
    NULL
};

SETFUNCERR(38, tx_l2pbm, pbmp_t)
SETFUNCERR(38, tx_utpbm, pbmp_t)
SETFUNCERR(38, tx_l3pbm, pbmp_t)
SETFUNCERR(38, tx_crc, int)
SETFUNCERR(38, tx_cos, int)
SETFUNCERR(38, tx_destmod, uint32)
SETFUNCERR(38, tx_destport, uint32)
SETFUNCERR(38, tx_opcode, uint32)
SETFUNCERR(38, tx_srcmod, uint32)
SETFUNCERR(38, tx_srcport, uint32)
SETFUNCERR(38, tx_prio, uint32)
SETFUNCERR(38, tx_pfm, uint32)

GETFUNCNULL(38, stat)
SETFUNCNULL2(38, stat)

dcb_op_t dcb38_op = {
    38,
    sizeof(dcb38_t),
    dcb38_rx_reason_maps,
    dcb0_rx_reason_map_get,
    ep_to_cpu_hdr38_rx_reasons_get,
    dcb38_init,
    dcb38_add_tx,
    dcb38_add_rx,
    dcb38_intr_info,
    dcb38_reqcount_set,
    dcb38_reqcount_get,
    dcb38_xfercount_get,
    dcb38_addr_set,
    dcb38_addr_get,
    dcb38_paddr_get,
    dcb38_done_set,
    dcb38_done_get,
    dcb38_sg_set,
    dcb38_sg_get,
    dcb38_chain_set,
    dcb38_chain_get,
    dcb38_reload_set,
    dcb38_reload_get,
    dcb38_desc_intr_set,
    dcb38_desc_intr_get,
    dcb38_tx_l2pbm_set,
    dcb38_tx_utpbm_set,
    dcb38_tx_l3pbm_set,
    dcb38_tx_crc_set,
    dcb38_tx_cos_set,
    dcb38_tx_destmod_set,
    dcb38_tx_destport_set,
    dcb38_tx_opcode_set,
    dcb38_tx_srcmod_set,
    dcb38_tx_srcport_set,
    dcb38_tx_prio_set,
    dcb38_tx_pfm_set,
    ep_to_cpu_hdr38_rx_untagged_get,
    ep_to_cpu_hdr38_rx_crc_get,
    ep_to_cpu_hdr38_rx_cos_get,
    ep_to_cpu_hdr38_rx_destmod_get,
    ep_to_cpu_hdr38_rx_destport_get,
    ep_to_cpu_hdr38_rx_opcode_get,
    ep_to_cpu_hdr38_rx_classtag_get,
    ep_to_cpu_hdr38_rx_matchrule_get,
    dcb38_rx_start_get,
    dcb38_rx_end_get,
    dcb38_rx_error_get,
    ep_to_cpu_hdr38_rx_prio_get,
    ep_to_cpu_hdr38_rx_reason_get,
    ep_to_cpu_hdr38_rx_reason_hi_get,
    ep_to_cpu_hdr38_rx_ingport_get,
    ep_to_cpu_hdr38_rx_srcport_get,
    ep_to_cpu_hdr38_rx_srcmod_get,
    ep_to_cpu_hdr38_rx_mcast_get,
    ep_to_cpu_hdr38_rx_vclabel_get,
    ep_to_cpu_hdr38_rx_mirror_get,
    ep_to_cpu_hdr38_rx_timestamp_get,
    ep_to_cpu_hdr38_rx_timestamp_upper_get,
    dcb38_hg_set,
    dcb38_hg_get,
    dcb38_stat_set,
    dcb38_stat_get,
    dcb38_purge_set,
    dcb38_purge_get,
    ep_to_cpu_hdr38_mhp_get,
    ep_to_cpu_hdr38_outer_vid_get,
    ep_to_cpu_hdr38_outer_pri_get,
    ep_to_cpu_hdr38_outer_cfi_get,
    ep_to_cpu_hdr38_rx_outer_tag_action_get,
    ep_to_cpu_hdr38_inner_vid_get,
    ep_to_cpu_hdr38_inner_pri_get,
    ep_to_cpu_hdr38_inner_cfi_get,
    ep_to_cpu_hdr38_rx_inner_tag_action_get,
    ep_to_cpu_hdr38_rx_bpdu_get,
    ep_to_cpu_hdr38_rx_l3_intf_get,
    ep_to_cpu_hdr38_rx_decap_tunnel_get,
    ep_to_cpu_hdr38_rx_switch_drop_get,
    ep_to_cpu_hdr38_olp_encap_oam_pkt_get,
    dcb38_read_ecc_error_get,
    dcb38_desc_remaining_get,
    dcb38_desc_remaining_set,
    dcb38_desc_status_get,
    dcb38_desc_status_set,
    NULL,
};
#endif /* BCM_TOMAHAWK3_SUPPORT */

#endif /* defined(BCM_ESW_SUPPORT) */

