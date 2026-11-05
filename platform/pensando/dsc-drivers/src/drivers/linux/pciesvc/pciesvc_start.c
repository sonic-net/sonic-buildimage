// SPDX-License-Identifier: GPL-2.0
/*
 * PCIESVC Library Loader - Start of Code Marker
 * Ensures pciesvc_start is placed before *all* other .text.* sections.
 */

__attribute__((section(".text.aaa_pciesvc_start")))
__attribute__((__noinline__))
__attribute__((__used__))
__attribute__((visibility("default")))
void pciesvc_start(void) { /* deliberately empty */ }
