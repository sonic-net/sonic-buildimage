/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019, Pensando Systems Inc.
 */

#ifndef PCIEMGR_STATS_DEF
#define PCIEMGR_STATS_DEF(st)
#endif

PCIEMGR_STATS_DEF(not_intr)
PCIEMGR_STATS_DEF(not_spurious)
PCIEMGR_STATS_DEF(not_polled)
PCIEMGR_STATS_DEF(not_cnt)
PCIEMGR_STATS_DEF(not_max)
PCIEMGR_STATS_DEF(not_cfgrd)
PCIEMGR_STATS_DEF(not_cfgwr)
PCIEMGR_STATS_DEF(not_memrd)
PCIEMGR_STATS_DEF(not_memwr)
PCIEMGR_STATS_DEF(not_iord)
PCIEMGR_STATS_DEF(not_iowr)
PCIEMGR_STATS_DEF(not_unknown)

#define notify_reason_stats not_rsrv0
PCIEMGR_STATS_DEF(not_rsrv0)
PCIEMGR_STATS_DEF(not_rsrv1)
PCIEMGR_STATS_DEF(not_msg)
PCIEMGR_STATS_DEF(not_unsupported)
PCIEMGR_STATS_DEF(not_pmv)
PCIEMGR_STATS_DEF(not_dbpmv)
PCIEMGR_STATS_DEF(not_atomic)
PCIEMGR_STATS_DEF(not_pmtmiss)
PCIEMGR_STATS_DEF(not_pmrmiss)
PCIEMGR_STATS_DEF(not_prtmiss)
PCIEMGR_STATS_DEF(not_bdf2vfidmiss)
PCIEMGR_STATS_DEF(not_prtoor)
PCIEMGR_STATS_DEF(not_vfidoor)
PCIEMGR_STATS_DEF(not_bdfoor)
PCIEMGR_STATS_DEF(not_pmrind)
PCIEMGR_STATS_DEF(not_prtind)
PCIEMGR_STATS_DEF(not_pmrecc)
PCIEMGR_STATS_DEF(not_prtecc)

PCIEMGR_STATS_DEF(ind_intr)
PCIEMGR_STATS_DEF(ind_spurious)
PCIEMGR_STATS_DEF(ind_polled)
PCIEMGR_STATS_DEF(ind_cfgrd)
PCIEMGR_STATS_DEF(ind_cfgwr)
PCIEMGR_STATS_DEF(ind_memrd)
PCIEMGR_STATS_DEF(ind_memwr)
PCIEMGR_STATS_DEF(ind_iord)
PCIEMGR_STATS_DEF(ind_iowr)
PCIEMGR_STATS_DEF(ind_unknown)

PCIEMGR_STATS_DEF(healthlog)

#undef PCIEMGR_STATS_DEF
