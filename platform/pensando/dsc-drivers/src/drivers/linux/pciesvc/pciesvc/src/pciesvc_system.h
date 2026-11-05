/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021-2022, Pensando Systems Inc.
 */

#ifndef __PCIESVC_SYSTEM_H__
#define __PCIESVC_SYSTEM_H__

/*
 * Some build environments bring a customized version of these
 * "system" functions (e.g. Linux kernel).  If building for
 * one of these environments build with -DPCIESVC_SYSTEM_EXTERN
 * and provide a matching "pciesvc_system_extern.h",
 * otherwise we pick up the local "system" functions.
 */
#ifdef PCIESVC_SYSTEM_EXTERN
#include "pciesvc_system_extern.h"
#else
#include "pciesvc_system_local.h"
#endif

#endif /* __PCIESVC_SYSTEM_H__ */
