/*
 * $Copyright: 2017-2024 Broadcom Inc. All rights reserved.
 * 
 * Permission is granted to use, copy, modify and/or distribute this
 * software under either one of the licenses below.
 * 
 * License Option 1: GPL
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2, as
 * published by the Free Software Foundation (the "GPL").
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License version 2 (GPLv2) for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * version 2 (GPLv2) along with this source code.
 * 
 * 
 * License Option 2: Broadcom Open Network Switch APIs (OpenNSA) license
 * 
 * This software is governed by the Broadcom Open Network Switch APIs license:
 * https://www.broadcom.com/products/ethernet-connectivity/software/opennsa $
 * 
 * 
 */

/*
 * Middle-driver for communication between Linux KNET driver and
 * drivers support Generic Netlink channel.
 *
 * This driver utilizes the NETIF and RX filter call-back functions
 * of Linux KNET driver to trasform the RX filter packets to Generic Netlink
 * packets for application usage.
 *
 * Current supported Generic Netlink kernel modules are:
 *
 * - psample module in kernel/linux/net/psample
 *
 *   Filter call-back function is registered for recevieing KNET filter
 *   created with description name 'psample'.
 *
 * - genl_packet module from Google
 *
 *   Filter call-back function is registered for recevieing KNET filter
 *   created with description name 'genl_packet'.
 *
 * This driver is also built with the DCB library as the helper for parsing
 * the RX packet meta data from the Linux KNET driver filter call-back function.
 * The environment DCBDIR must be set to indicate the directroy of the DCB
 * library.
 *
 * The module can be built from the standard Linux user mode target
 * directories using the following command (assuming bash), e.g.
 *
 *   cd $SDK/systems/linux/user/gto-2_6
 *   BUILD_BCM_GENL=1 make -s mod
 *
 */

#include <gmodule.h> /* Must be included first */
#include <linux-bde.h>
#include "bcm-genl-dev.h"
#include "bcm-genl-netif.h"
#include "bcm-genl-psample.h"
#include "bcm-genl-packet.h"

MODULE_AUTHOR("Broadcom Corporation");
MODULE_DESCRIPTION("Broadcom Linux KNET Call-Back Driver for GenLink");
MODULE_LICENSE("GPL");

int debug;
LKM_MOD_PARAM(debug, "i", int, 0);
MODULE_PARM_DESC(debug,
"Debug level (default 0)");

/* Module Information */
#define MODULE_MAJOR 0
#define MODULE_NAME "linux-bcm-genl"

#define BCMGENL_PROCFS_PATH "bcm/genl"

/* driver proc entry root */
static struct proc_dir_entry *bcmgenl_proc_root = NULL;

/*
 * dev Proc Read Entry
 */
static int
proc_dev_show(struct seq_file *m, void *v)
{
    int dev_no;
    int dcb_type, dcb_size;

    for (dev_no = 0; dev_no < LINUX_BDE_MAX_DEVICES; dev_no++) {
        if (bcmgenl_dev_dcb_info_get(dev_no, &dcb_type, &dcb_size) == 0) {
            seq_printf(m, "Device number %d:\n", dev_no);
            seq_printf(m, "  dcb_type:    %d\n", dcb_type);
            seq_printf(m, "  dcb_size:    %d\n", dcb_size);
        }
    }
    return 0;
}

static int
proc_dev_open(struct inode *inode, struct file *file)
{
    return single_open(file, proc_dev_show, NULL);
}

struct proc_ops proc_dev_file_ops = {
    PROC_OWNER(THIS_MODULE)
    .proc_open =       proc_dev_open,
    .proc_read =       seq_read,
    .proc_lseek =      seq_lseek,
    .proc_write =      NULL,
    .proc_release =    single_release,
};

/*
 * netif Proc Read Entry
 */
static int
netif_show(void *cb_data, bcmgenl_netif_t *netif)
{
    struct seq_file *m = (struct seq_file *)cb_data;

    seq_printf(m, "  %-14s %-14d %d\n",
               netif->dev->name, netif->port, netif->dev->ifindex);
    return 0;
}

static int
proc_netif_show(struct seq_file *m, void *v)
{
    if (bcmgenl_netif_num_get() == 0) {
        seq_printf(m, "No interfaces are available\n");
        return 0;
    }
    seq_printf(m, "  Interface      logical port   ifindex\n");
    seq_printf(m, "-------------    ------------   -------\n");
    bcmgenl_netif_search(NULL, netif_show, (void *)m);
    return 0;
}

static int
proc_netif_open(struct inode *inode, struct file *file)
{
    return single_open(file, proc_netif_show, NULL);
}

struct proc_ops proc_netif_file_ops = {
    PROC_OWNER(THIS_MODULE)
    .proc_open =       proc_netif_open,
    .proc_read =       seq_read,
    .proc_lseek =      seq_lseek,
    .proc_write =      NULL,
    .proc_release =    single_release,
};

static int
bcmgenl_proc_init(void)
{
    struct proc_dir_entry *entry;

    /* Initialize proc files */
    bcmgenl_proc_root = proc_mkdir(BCMGENL_PROCFS_PATH, NULL);

    /* create procfs for getting netdev mapping */
    PROC_CREATE(entry, "netif", 0666, bcmgenl_proc_root, &proc_netif_file_ops);
    if (entry == NULL) {
        return -1;
    }

    /* create procfs for generic stats */
    PROC_CREATE(entry, "dev", 0666, bcmgenl_proc_root, &proc_dev_file_ops);
    if (entry == NULL) {
        return -1;
    }

    return 0;
}

static int
bcmgenl_proc_cleanup(void)
{
    remove_proc_entry("netif", bcmgenl_proc_root);
    remove_proc_entry("dev", bcmgenl_proc_root);
    remove_proc_entry(BCMGENL_PROCFS_PATH, NULL);
    return 0;
}

/*
 * Get statistics.
 * % cat /proc/linux-bcm-genl
 */
static int
_pprint(struct seq_file *m)
{
    pprintf(m, "Broadcom Linux KNET Call-Back: genlink\n");

    return 0;
}

static int
_cleanup(void)
{
    bcmgenl_psample_cleanup();
#ifdef BUILD_GENL_PACKET
    bcmgenl_packet_cleanup();
#endif
    bcmgenl_netif_cleanup();
    bcmgenl_dev_cleanup();

    bcmgenl_proc_cleanup();

    return 0;
}

static int
_init(void)
{
    bcmgenl_proc_init();

    bcmgenl_dev_init();
    bcmgenl_netif_init();

    bcmgenl_psample_init(BCMGENL_PROCFS_PATH);
#ifdef BUILD_GENL_PACKET
    bcmgenl_packet_init(BCMGENL_PROCFS_PATH);
#endif

    return 0;
}

static gmodule_t _gmodule = {
    name: MODULE_NAME,
    major: MODULE_MAJOR,
    init: _init,
    cleanup: _cleanup,
    pprint: _pprint,
    ioctl: NULL,
    open: NULL,
    close: NULL,
};

gmodule_t*
gmodule_get(void)
{
    EXPORT_NO_SYMBOLS;
    return &_gmodule;
}
