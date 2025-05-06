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

#include "kpcimgr_api.h"
#include "pciesvc.h"
#include "version.h"

MODULE_LICENSE("GPL");
MODULE_VERSION(__stringify(PCIESVC_VERSION_MAJ) "."
		__stringify(PCIESVC_VERSION_MIN));
MODULE_INFO(build, PCIESVC_VERSION);
MODULE_INFO(intree, "Y"); /* no out-of-tree module taints kernel */

static int relocate = 0;
#ifdef DEBUG_KPCIMGR
module_param(relocate, int, 0600);
MODULE_PARM_DESC(relocate, "specifies whether or not to relocate module");
#endif

static int __init pciesvc_dev_init(void)
{
	struct kpcimgr_entry_points_t *kpci_get_entry_points(void);
	struct kpcimgr_entry_points_t *ep;
	int ret;

	ep = kpci_get_entry_points();

	/* call to Pensando SOC driver to copy the code to persistent memory */
	ret = kpcimgr_module_register(THIS_MODULE, ep, relocate);
#ifdef PEN_COMPAT_V2
	if (ret < 0) {
		/* attempt compat registration, some entry_point[] unused */
		ep->expected_mgr_version = 2;
		ret = kpcimgr_module_register(THIS_MODULE, ep, relocate);
	}
#endif

	return ret;
}

module_init(pciesvc_dev_init);

static void __exit pciesvc_dev_detach(void)
{
}
module_exit(pciesvc_dev_detach);
