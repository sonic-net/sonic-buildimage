// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2021, 2022, Oracle and/or its affiliates.
 */

/*
 * PCIESVC Library Loader - End of Code Marker
 *
 * This object file is last in the module link order and so the symbol
 * pciesvc_end gives us the address of the end of the code section in
 * the module. What follows are the various data sections.
 *
 * The reason this is needed is to be able to examine the code in in
 * kpcimgr_module_register() without accidentally looking at data. At
 * the time kpcimgr_module_register() is called, the kernel has
 * completely finished loading the module and all the meta data (i.e.,
 * section headers etc) have been discarded, and so there is nothing
 * to tell us where the code ends.
 *
 * Author: rob.gardner@oracle.com
 */
__attribute__((__noinline__)) void pciesvc_end(void)
{
}
