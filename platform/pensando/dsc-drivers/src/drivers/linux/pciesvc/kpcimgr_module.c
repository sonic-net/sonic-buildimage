// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2021, 2022, Oracle and/or its affiliates.
 */

/*
 * PCIESVC Library Loader
 *
 * Author: rob.gardner@oracle.com
 */

#include <linux/init.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/ctype.h>

MODULE_LICENSE("GPL");

#include "kpcimgr_api.h"
#include "version.h"

static int relocate = 0;
#ifdef DEBUG_KPCIMGR
module_param(relocate, int, 0600);
MODULE_PARM_DESC(relocate, "specifies whether or not to relocate module");
#endif

extern char pciesvc_end;
extern void kpcimgr_init_intr(void *);
extern void kpcimgr_init_fn(void *);
extern void kpcimgr_version_fn(char **);
extern void kpcimgr_init_poll(void *);
extern void pciesvc_shut(int);
extern void kpcimgr_poll(kstate_t *, int, int);
extern unsigned long kpcimgr_get_holding_pen(unsigned long, unsigned int);
extern int kpcimgr_ind_intr(void *, int);
extern int kpcimgr_not_intr(void *, int);
extern void kpcimgr_undefined_entry(void);
extern int pciesvc_sysfs_cmd_read(void *, char *, int *);
extern int pciesvc_sysfs_cmd_write(void *, char *, size_t, int *);

extern int pciesvc_version_major;
extern int pciesvc_version_minor;

static int __init pciesvc_dev_init(void)
{
	struct kpcimgr_entry_points_t ep;
	int i, ret = 0;

	/* initialize entry_points struct via executable code so that
	 * PC relative relocations are generated */
	ep.expected_mgr_version = 3;
	ep.lib_version_major = pciesvc_version_major;
	ep.lib_version_minor = pciesvc_version_minor;
	ep.code_end = &pciesvc_end;

	for (i=0; i<K_NUM_ENTRIES; i++)
		ep.entry_point[i] = kpcimgr_undefined_entry;

	ep.entry_point[K_ENTRY_INIT_INTR] = kpcimgr_init_intr;
	ep.entry_point[K_ENTRY_INIT_POLL] = kpcimgr_init_poll;
	ep.entry_point[K_ENTRY_SHUT] = pciesvc_shut;
	ep.entry_point[K_ENTRY_POLL] = kpcimgr_poll;
	ep.entry_point[K_ENTRY_HOLDING_PEN] = kpcimgr_get_holding_pen;
	ep.entry_point[K_ENTRY_INDIRECT_INTR] = kpcimgr_ind_intr;
	ep.entry_point[K_ENTRY_NOTIFY_INTR] = kpcimgr_not_intr;
	ep.entry_point[K_ENTRY_INIT_FN] = kpcimgr_init_fn;
	ep.entry_point[K_ENTRY_CMD_READ] = pciesvc_sysfs_cmd_read;
	ep.entry_point[K_ENTRY_CMD_WRITE] = pciesvc_sysfs_cmd_write;
	ep.entry_point[K_ENTRY_GET_VERSION] = kpcimgr_version_fn;

	/* call to Pensando SOC driver to copy the code to persistent memory */
	ret = kpcimgr_module_register(THIS_MODULE, &ep, relocate);
#ifdef PEN_COMPAT_V2
	if (ret < 0) {
		/* attempt compat registration, some entry_point[] unused */
		ep.expected_mgr_version = 2;
		ret = kpcimgr_module_register(THIS_MODULE, &ep, relocate);
	}
#endif

	return ret;
}

module_init(pciesvc_dev_init);

static void __exit pciesvc_dev_detach(void)
{
}
module_exit(pciesvc_dev_detach);
