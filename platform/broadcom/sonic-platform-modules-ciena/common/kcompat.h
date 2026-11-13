/* SPDX-License-Identifier: GPL-2.0 */
/*
 * kcompat.h - Kernel version compatibility layer for Ciena drivers
 *
 * This header provides compatibility macros so that the same driver
 * source can compile on both OneOS (kernel 6.1) and SONiC (kernel 6.12).
 *
 * Include this header early (or use -include via CFLAGS) so that all
 * source files pick up the correct API wrappers automatically.
 *
 * Copyright (C) 2025 Ciena Corporation
 */

#ifndef _KCOMPAT_H
#define _KCOMPAT_H

#include <linux/version.h>

/* -------------------------------------------------------------------------
 * Thermal API compatibility (6.1 → 6.2+)
 *
 * Kernel 6.2+: thermal_zone_device_priv() replaces tz->devdata
 * Kernel 6.2+: thermal_zone_device() replaces &tz->device
 * Kernel 6.2+: thermal_zone_device_register_with_trips() drops mask param
 * Kernel 6.2+: struct thermal_trip gains .flags field
 * ------------------------------------------------------------------------- */
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 2, 0)

#define thermal_zone_device_priv(tz)    ((tz)->devdata)

static inline struct device *thermal_zone_device(struct thermal_zone_device *tz)
{
	return &tz->device;
}

#define thermal_zone_device_register_with_trips(name, trips, ntrips, devdata, ops, tzp, pd, poll) \
	thermal_zone_device_register_with_trips(name, trips, ntrips, \
		(1 << (ntrips)) - 1, devdata, ops, tzp, pd, poll)

#ifndef THERMAL_TRIP_FLAG_RW_TEMP
#define THERMAL_TRIP_FLAG_RW_TEMP  0
#endif

#endif /* < 6.2 */

/* -------------------------------------------------------------------------
 * Thermal trip priv field (6.1 → 6.10+)
 *
 * Kernel 6.10+: struct thermal_trip has a .priv field and macros to
 * convert between integer trip indices and the priv pointer.
 * ------------------------------------------------------------------------- */
#ifndef THERMAL_TRIP_PRIV_TO_INT
#define THERMAL_TRIP_PRIV_TO_INT(_val_)	(uintptr_t)(_val_)
#define THERMAL_INT_TO_TRIP_PRIV(_val_)	(void *)(uintptr_t)(_val_)
#endif

/* -------------------------------------------------------------------------
 * bus_type const qualifier (6.1 → 6.3+)
 *
 * Kernel 6.3+: bus_type pointers became const in many driver APIs.
 * ------------------------------------------------------------------------- */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 3, 0)
#define CIENA_BUS_TYPE_PTR   const struct bus_type *
#else
#define CIENA_BUS_TYPE_PTR   struct bus_type *
#endif

#endif /* _KCOMPAT_H */
