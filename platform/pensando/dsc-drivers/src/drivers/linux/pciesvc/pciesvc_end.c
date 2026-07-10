// SPDX-License-Identifier: GPL-2.0
/*
 * PCIESVC Library Loader - End of Code Marker
 * Ensures pciesvc_end is placed after *all* other .text.* sections.
 */

__attribute__((section(".text.zzz_pciesvc_end")))
__attribute__((__noinline__))
__attribute__((__used__))
__attribute__((visibility("default")))
void pciesvc_end(void) { /* deliberately empty */ }
