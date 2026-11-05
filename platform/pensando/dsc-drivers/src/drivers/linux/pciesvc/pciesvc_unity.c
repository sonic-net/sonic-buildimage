// SPDX-License-Identifier: GPL-2.0
/*
 * PCIESVC Unity Build
 *
 * This file includes all pciesvc .c source files into a single translation
 * unit. This eliminates cross-file function calls (BL instructions) and
 * external data references (ADRP instructions), making the compiled code
 * fully relocatable for kexec scenarios.
 *
 * The order of includes matters:
 * 1. pciesvc_start.c - code start marker
 * 2. Library files (pciesvc/src/*.c) - core functionality
 * 3. Interface files - kpci_*.c, kpcimgr_module.c, kpcinterface.c
 * 4. pciesvc_end.c - code end marker
 *
 * Note: kpci_entry.S (assembly) must be compiled separately.
 *
 * Copyright (c) 2024 Pensando Systems, AMD
 */

/*
 * Prevent multiple definition errors by ensuring all symbols are
 * either static or uniquely named within this translation unit.
 */

/* Start marker - must be first */
#include "pciesvc_start.c"

/* ==== pciesvc library files (pciesvc/src/) ==== */

/* Low-level utilities first */
#include "pciesvc/src/printf.c"
#include "pciesvc/src/log.c"

/* Hardware abstraction */
#include "pciesvc/src/portcfg.c"
#include "pciesvc/src/intrutils.c"
#include "pciesvc/src/hdrt.c"

/* PMT (PCIe Memory Translation) */
#include "pciesvc/src/pmt.c"
#include "pciesvc/src/prt.c"

/* TLP (Transaction Layer Packet) handling */
#include "pciesvc/src/pcietlp.c"

/* BAR (Base Address Register) management */
#include "pciesvc/src/bar.c"

/* Configuration space */
#include "pciesvc/src/cfgspace.c"
#include "pciesvc/src/cfg.c"

/* Device management */
#include "pciesvc/src/pciehwdev.c"
#include "pciesvc/src/vpd.c"

/* Interrupt handling */
#include "pciesvc/src/intr.c"
#include "pciesvc/src/indirect.c"
#include "pciesvc/src/notify.c"

/* Request/reset handling */
#include "pciesvc/src/req_int.c"
#include "pciesvc/src/reset.c"

/* Device emulation */
#include "pciesvc/src/serial.c"
#include "pciesvc/src/virtio.c"

/* Command interface */
#include "pciesvc/src/cmd.c"

/* Main pciesvc interface */
#include "pciesvc/src/pciesvc.c"

/* ==== Top-level interface files ==== */

/*
 * Order matters here: kpci_get_entry.c uses functions defined in
 * kpcinterface.c, kpcimgr_module.c, and kpci_kexec.c, so those must
 * come first.
 */

/* Kexec support - defines kpcimgr_get_holding_pen */
#include "kpci_kexec.c"

/* Test/debug utilities */
#include "kpci_test.c"

/* Kernel interface - defines kpcimgr_init_fn, kpcimgr_init_intr, etc. */
#include "kpcinterface.c"

/* Module init - defines kpcimgr_version_fn */
#include "kpcimgr_module.c"

/* Entry points - uses all the above functions */
#include "kpci_get_entry.c"

/* End marker - must be last */
#include "pciesvc_end.c"
