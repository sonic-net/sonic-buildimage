/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2017-2018,2021-2022, Pensando Systems Inc.
 */

#ifndef __PRT_H__
#define __PRT_H__

union prt_u; typedef union prt_u prt_t;
int prt_alloc(const int n);
void prt_free(const int prtbase, const int prtcount);
void prt_get(const int prti, prt_t *prt);
void prt_set(const int prti, const prt_t *prt);

int pciehw_prt_load(const int prtbase, const int prtcount);
void pciehw_prt_unload(const int prtbase, const int prtcount);

#endif /* __PRT_H__ */
