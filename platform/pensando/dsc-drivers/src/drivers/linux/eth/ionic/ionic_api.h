/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (c) 2017 - 2022 Pensando Systems, Inc.  All rights reserved. */

#ifndef IONIC_API_H
#define IONIC_API_H

#include <linux/completion.h>
#include <linux/netdevice.h>
#include <linux/types.h>

#include "ionic_if.h"
#include "ionic_regs.h"

/**
 * IONIC_API_VERSION - Version number of this interface
 *
 * Any interface changes to this interface must also change the version.
 *
 * If netdev and other (eg, rdma) drivers are compiled from different sources,
 * they are compatible only if IONIC_API_VERSION is statically the same in both
 * sources.  Drivers must have matching values of IONIC_API_VERSION at compile
 * time, to be considered compatible at run time.
 */
#define IONIC_API_VERSION "8"

struct dentry;

/**
 * struct ionic_devinfo - device information
 * @asic_type:		Device ASIC type code
 * @asic_rev:		Device ASIC revision code
 * @fw_version:		Device firmware version, as a string
 * @serial_num:		Device serial number, as a string
 */
struct ionic_devinfo {
	u8 asic_type;
	u8 asic_rev;
	char fw_version[IONIC_DEVINFO_FWVERS_BUFLEN + 1];
	char serial_num[IONIC_DEVINFO_SERIAL_BUFLEN + 1];
};

/**
 * enum ionic_api_prsn - personalities that can be applied to a lif
 * @IONIC_PRSN_NONE:	No personality assigned
 * @IONIC_PRSN_ETH:	Ethernet NIC personality assigned
 * @IONIC_PRSN_RDMA:	RDMA HCA personality assigned
 */
enum ionic_api_prsn {
	IONIC_PRSN_NONE = 0,
	IONIC_PRSN_ETH,
	IONIC_PRSN_RDMA,
};

/**
 * ionic_get_handle_from_netdev() - Get a handle if the netdev is ionic
 * @netdev:		Net device to check
 * @api_version:	IONIC_API_VERSION
 * @prsn:		Personality to apply
 *
 * This returns an opaque handle if and only if the netdev was created
 * by the ionic driver and the api version matches as described
 * above for IONIC_API_VERSION.
 *
 * Return: Handle, if netdev is a compatible ionic device, or ERR_PTR(error)
 */
void *ionic_get_handle_from_netdev(struct net_device *netdev,
				   const char *api_version,
				   enum ionic_api_prsn prsn);

/**
 * ionic_api_stay_registered() - stay registered through net interface changes
 * @handle:		Handle to lif
 *
 * Return: true if the child device should ignore net deregistration events
 */
bool ionic_api_stay_registered(void *handle);

/**
 * ionic_api_request_reset() - request reset or disable the device or lif
 * @handle:		Handle to lif
 *
 * The reset will be carried out asynchronously.  If it succeeds, then the
 * callback specified in ionic_api_set_private() will be called.
 */
void ionic_api_request_reset(void *handle);

/**
 * ionic_api_get_private() - Get private data associated with the lif
 * @handle:		Handle to lif
 * @prsn:		Personality to which the private data applies
 *
 * Get the private data of some kind.  The private data may be, for example, an
 * instance of an rdma device for this lif.
 *
 * Return: private data or NULL
 */
void *ionic_api_get_private(void *handle, enum ionic_api_prsn prsn);

/**
 * ionic_api_set_private() - Set private data associated with the lif
 * @handle:		Handle to lif
 * @priv:		Private data or NULL
 * @reset_cb:		Callback if device has been disabled or reset
 * @prsn:		Personality to which the private data applies
 *
 * Set the private data of some kind.  The private data may be, for example, an
 * instance of an rdma device for this lif.
 *
 * This will fail if private data is already set for that personality.
 *
 * Return: zero or negative error status
 */
int ionic_api_set_private(void *handle, void *priv,
			  void (*reset_cb)(void *priv),
			  enum ionic_api_prsn prsn);

/**
 * ionic_api_clear_private() - Clear private data associated with the lif
 * @handle:		Handle to lif
 */
static inline void ionic_api_clear_private(void *handle)
{
	(void)ionic_api_set_private(handle, NULL, NULL, IONIC_PRSN_NONE);
}

/**
 * ionic_api_get_device() - Get the underlying device
 * @handle:		Handle to lif
 *
 * Return: pointer to underlying OS struct device associated with the lif
 */
struct device *ionic_api_get_device(void *handle);

/**
 * ionic_api_get_devinfo() - Get device information
 * @handle:		Handle to lif
 *
 * Return: pointer to device information
 */
const struct ionic_devinfo *ionic_api_get_devinfo(void *handle);

/**
 * ionic_api_get_debug_ctx() - Get the debug context (if any) for the lif
 * @handle:		Handle to lif
 *
 * This is the directory entry of the LIF in debugfs.
 *
 * Return: debug context for the lif or NULL
 */
struct dentry *ionic_api_get_debug_ctx(void *handle);

/**
 * ionic_api_get_identity() - Get result of device identification
 * @handle:		Handle to lif
 * @lif_index:		This lif index
 *
 * Return: pointer to result of identification
 */
const union ionic_lif_identity *ionic_api_get_identity(void *handle,
						       int *lif_index);

/**
 * ionic_api_get_intr() - Reserve a device interrupt index
 * @handle:		Handle to lif
 * @irq:		OS interrupt number returned
 *
 * Reserve an interrupt index, and indicate the irq number for that index.
 *
 * Return: interrupt index or negative error status
 */
int ionic_api_get_intr(void *handle, int *irq);

/**
 * ionic_api_put_intr() - Release a device interrupt index
 * @handle:		Handle to lif
 * @intr:		Interrupt index
 *
 * Mark the interrupt index unused so that it can be reserved again.
 */
void ionic_api_put_intr(void *handle, int intr);

/**
 * ionic_api_get_cmb() - Reserve cmb pages
 * @handle:		Handle to lif
 * @pgid:		First page index
 * @pgaddr:		First page bus addr (contiguous)
 * @order:		Log base two number of pages (PAGE_SIZE)
 *
 * Return: zero or negative error status
 */
int ionic_api_get_cmb(void *handle, u32 *pgid, phys_addr_t *pgaddr, int order);

/**
 * ionic_api_put_cmb() - Release cmb pages
 * @handle:		Handle to lif
 * @pgid:		First page index
 * @order:		Log base two number of pages (PAGE_SIZE)
 */
void ionic_api_put_cmb(void *handle, u32 pgid, int order);

/**
 * ionic_api_kernel_dbpage() - Get mapped doorbell page for use in kernel space
 * @handle:		Handle to lif
 * @intr_ctrl:		Interrupt control registers
 * @dbid:		Doorbell id for use in kernel space
 * @dbpage:		One ioremapped doorbell page for use in kernel space
 *
 * This also provides mapped interrupt control registers.
 *
 * The id and page returned here refer to the doorbell page reserved for use in
 * kernel space for this lif.  For user space, use ionic_api_get_dbid to
 * allocate a doorbell id for exclusive use by a process.
 */
void ionic_api_kernel_dbpage(void *handle,
			     struct ionic_intr __iomem **intr_ctrl,
			     u32 *dbid, u64 __iomem **dbpage);

/**
 * ionic_api_get_dbid() - Reserve a doorbell id
 * @handle:		Handle to lif
 * @dbid:		Doorbell id
 * @addr:		Phys address of doorbell page
 *
 * Reserve a doorbell id.  This corresponds with exactly one doorbell page at
 * an offset from the doorbell page base address, that can be mapped into a
 * user space process.
 *
 * Return: zero on success or negative error status
 */
int ionic_api_get_dbid(void *handle, u32 *dbid, phys_addr_t *addr);

/**
 * ionic_api_put_dbid() - Release a doorbell id
 * @handle:		Handle to lif
 * @dbid:		Doorbell id
 *
 * Mark the doorbell id unused, so that it can be reserved again.
 */
void ionic_api_put_dbid(void *handle, int dbid);

/**
 * struct ionic_admin_ctx - Admin command context
 * @work:		Work completion wait queue element
 * @cmd:		Admin command (64B) to be copied to the queue
 * @comp:		Admin completion (16B) copied from the queue
 */
struct ionic_admin_ctx {
	struct completion work;
	union ionic_adminq_cmd cmd;
	union ionic_adminq_comp comp;
};

/**
 * ionic_api_adminq_post() - Post an admin command
 * @handle:		Handle to lif
 * @ctx:		API admin command context
 *
 * Post the command to an admin queue in the ethernet driver.  If this command
 * succeeds, then the command has been posted, but that does not indicate a
 * completion.  If this command returns success, then the completion callback
 * will eventually be called.
 *
 * Return: zero or negative error status
 */
int ionic_api_adminq_post(void *handle, struct ionic_admin_ctx *ctx);

/**
 * ionic_error_to_errno() - Transform ionic_if errors to os errno
 * @code:		Ionic error number
 *
 * Return:		Negative OS error number or zero
 */
int ionic_error_to_errno(enum ionic_status_code code);

#endif /* IONIC_API_H */
