/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2017-2018,2021, Pensando Systems Inc.
 */

#ifndef __PCIESVC_PCIEHW_H__
#define __PCIESVC_PCIEHW_H__

#ifdef __cplusplus
extern "C" {
#if 0
} /* close to calm emacs autoindent */
#endif
#endif

#define PCIEHW_NPORTS   8
/*
#ifdef SALINA
#define PCIEHW_NDEVS    4150
#define PCIEHW_NDEVS_HI 4150
#else
*/
#define PCIEHW_NDEVS    1024
#define PCIEHW_NDEVS_HI 2080
/* #endif */
#define PCIEHW_CFGSHIFT 11
#define PCIEHW_CFGSZ    (1 << PCIEHW_CFGSHIFT)
#define PCIEHW_NROMSK   128
#define PCIEHW_NPMT     PMT_COUNT
#define PCIEHW_NPRT     PRT_COUNT
#define PCIEHW_NBAR     6               /* 6 cfgspace BARs */

#ifdef __cplusplus
}
#endif

#endif /* __PCIESVC_PCIEHW_H__ */
