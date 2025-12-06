/*! \file bcmdrd_config.h
 *
 * This config file defines all compilation-time specifications for
 * the BCMDRD.
 *
 * Reasonable defaults are provided for all configuration options
 * where appropriate.
 *
 * You need not edit this file directly to change your configuration,
 * nor is modifying this file advised -- so doing will require
 * manually merging whenever the BCMDRD is upgraded.
 *
 * You should provide your own configuration options or overrides
 * through a combination of:
 *
 *      1. The compiler command line, such as -D{OPTION}={VALUE}
 *
 *      2. Create your own custom configuration file:
 *         a) Create a file called 'bcmdrd_custom_config.h'
 *         b) Define all custom settings, using this file as
 *            the reference
 *         c) Add -DBCMDRD_INCLUDE_CUSTOM_CONFIG to your
 *            compilation
 *         d) Make sure the compilation include path includes
 *            'bcmdrd_custom_config.h'
 *
 */
/*
 *
 * Copyright 2018-2025 Broadcom. All rights reserved.
 * The term 'Broadcom' refers to Broadcom Inc. and/or its subsidiaries.
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License 
 * version 2 as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * A copy of the GNU General Public License version 2 (GPLv2) can
 * be found in the LICENSES folder.
 */

#ifndef BCMDRD_CONFIG_H
#define BCMDRD_CONFIG_H

/*
 * Include system config file if specified:
 */
#ifdef BCMDRD_INCLUDE_CUSTOM_CONFIG
#include <bcmdrd_custom_config.h>
#endif

/*
 * OPTIONAL configuration and feature values.
 * Defaults are provided for all non-specified values.
 */

/*! Maximum number of chips supported. */
#ifndef BCMDRD_CONFIG_MAX_UNITS
#define BCMDRD_CONFIG_MAX_UNITS                    8
#endif

/*! Maximum number of ports per chip supported. */
#ifndef BCMDRD_CONFIG_MAX_PORTS
#define BCMDRD_CONFIG_MAX_PORTS                    2048
#endif

/*! Maximum number of pipes per chip supported. */
#ifndef BCMDRD_CONFIG_MAX_PIPES
#define BCMDRD_CONFIG_MAX_PIPES                    96
#endif

/*! Maximum size of physical table entries (in bytes). */
#ifndef BCMDRD_CONFIG_MAX_PT_ENTRY_SIZE
#define BCMDRD_CONFIG_MAX_PT_ENTRY_SIZE            128
#endif

/*! Maximum number of interrupt lines per chip supported. */
#ifndef BCMDRD_CONFIG_MAX_IRQ_LINES
#define BCMDRD_CONFIG_MAX_IRQ_LINES                16
#endif

/*! Direct access to memory-mapped registers. */
#ifndef BCMDRD_CONFIG_MEMMAP_DIRECT
#define BCMDRD_CONFIG_MEMMAP_DIRECT                0
#endif

/*!
 * \brief Include chip symbol tables.
 *
 * No symbolic debugging (register/memory names) will be available
 * without this defined.
 *
 * This define is required to get any symbols at all.
 *
 * Symbols tables are required for normal SDK operation, but may be
 * excluded for small footprint base driver applications.
 *
 * If you only wish to include symbols for a subset of chips in the
 * system (probably for code space reasons), you can define the
 * following for each chip whose symbols you wish to EXCLUDE:
 *
 *       BCMDRD_CONFIG_EXCLUDE_CHIP_SYMBOLS_<CHIP>
 *
 */
#ifndef BCMDRD_CONFIG_INCLUDE_CHIP_SYMBOLS
#define BCMDRD_CONFIG_INCLUDE_CHIP_SYMBOLS         1
#endif

/*!
 * \brief Include register and memory field information.
 *
 * This provides encoding, decoding, and displaying individual field
 * values for each register and memory.
 *
 * Requires more code space than just the chip symbols alone.
 *
 * Symbols field information is required for normal SDK operation, but
 * may be excluded for small footprint base driver applications.
 *
 * The per-chip exclusion define
 * (BCMDRD_CONFIG_EXCLUDE_FIELD_INFO_<CHIP>) also applies.
 */
#ifndef BCMDRD_CONFIG_INCLUDE_FIELD_INFO
#define BCMDRD_CONFIG_INCLUDE_FIELD_INFO           1
#endif

/*!
 * \brief Include alternative symbol names for registers and memories.
 *
 * Mainly for internal Broadcom use, so you can safely leave this
 * option off.
 */
#ifndef BCMDRD_CONFIG_INCLUDE_ALIAS_NAMES
#define BCMDRD_CONFIG_INCLUDE_ALIAS_NAMES          1
#endif

/*!
 * \brief Include field size checks for registers and memories.
 *
 * This option adds compile-time checks for field values exceeding the
 * size of the field being assigned. The check in mainly intended for
 * internal use, and it may trigger warnings from various memory
 * sanity checker tools.
 */
#ifndef BCMDRD_CONFIG_INCLUDE_FIELD_CHECKS
#define BCMDRD_CONFIG_INCLUDE_FIELD_CHECKS         0
#endif

#endif /* BCMDRD_CONFIG_H */

#ifdef CONFIG_OPTION
#ifdef BCMDRD_INCLUDE_CUSTOM_CONFIG
CONFIG_OPTION(BCMDRD_INCLUDE_CUSTOM_CONFIG)
#endif
#ifdef BCMDRD_CONFIG_MAX_UNITS
CONFIG_OPTION(BCMDRD_CONFIG_MAX_UNITS)
#endif
#ifdef BCMDRD_CONFIG_MAX_PORTS
CONFIG_OPTION(BCMDRD_CONFIG_MAX_PORTS)
#endif
#ifdef BCMDRD_CONFIG_MAX_PIPES
CONFIG_OPTION(BCMDRD_CONFIG_MAX_PIPES)
#endif
#ifdef BCMDRD_CONFIG_MAX_PT_ENTRY_SIZE
CONFIG_OPTION(BCMDRD_CONFIG_MAX_PT_ENTRY_SIZE)
#endif
#ifdef BCMDRD_CONFIG_MAX_IRQ_LINES
CONFIG_OPTION(BCMDRD_CONFIG_MAX_IRQ_LINES)
#endif
#ifdef BCMDRD_CONFIG_MEMMAP_DIRECT
CONFIG_OPTION(BCMDRD_CONFIG_MEMMAP_DIRECT)
#endif
#ifdef BCMDRD_CONFIG_INCLUDE_CHIP_SYMBOLS
CONFIG_OPTION(BCMDRD_CONFIG_INCLUDE_CHIP_SYMBOLS)
#endif
#ifdef BCMDRD_CONFIG_INCLUDE_FIELD_INFO
CONFIG_OPTION(BCMDRD_CONFIG_INCLUDE_FIELD_INFO)
#endif
#ifdef BCMDRD_CONFIG_INCLUDE_ALIAS_NAMES
CONFIG_OPTION(BCMDRD_CONFIG_INCLUDE_ALIAS_NAMES)
#endif
#ifdef BCMDRD_CONFIG_INCLUDE_FIELD_CHECKS
CONFIG_OPTION(BCMDRD_CONFIG_INCLUDE_FIELD_CHECKS)
#endif
#endif /* CONFIG_OPTION */
#include "bcmdrd_config_chips.h"
