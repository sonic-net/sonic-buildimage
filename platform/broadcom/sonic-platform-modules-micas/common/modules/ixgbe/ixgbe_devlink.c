/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (C) 1999 - 2026 Intel Corporation */

#include "ixgbe.h"
#include "ixgbe_devlink.h"
#include "ixgbe_fw_update.h"

#if IS_ENABLED(CONFIG_NET_DEVLINK)

#ifdef HAVE_DEVLINK_INFO_GET

/* context for devlink info version reporting */
struct ixgbe_info_ctx {
	char buf[128];
	struct ixgbe_orom_info pending_orom;
	struct ixgbe_nvm_info pending_nvm;
	struct ixgbe_netlist_info pending_netlist;
	struct ixgbe_hw_dev_caps dev_caps;
};

/* The following functions are used to format specific strings for various
 * devlink info versions. The ctx parameter is used to provide the storage
 * buffer, as well as any ancillary information calculated when the info
 * request was made.
 *
 * If a version does not exist, for example when attempting to get the
 * inactive version of flash when there is no pending update, the function
 * should leave the buffer in the ctx structure empty.
 */

/**
 * ixgbe_info_get_dsn - get Device-Serial-Number
 * @adapter: adapter instance
 * @ctx: context providing buffer and additional information for the request
 *
 * Format DSN string and put it into the context buffer
 * in the Big Endian format.
 */
static void ixgbe_info_get_dsn(struct ixgbe_adapter *adapter,
			       struct ixgbe_info_ctx *ctx)
{
	u8 dsn[8];

	/* Copy the DSN into an array in Big Endian format */
	put_unaligned_be64(pci_get_dsn(adapter->pdev), dsn);

	snprintf(ctx->buf, sizeof(ctx->buf), "%8phD", dsn);
}

/**
 * ixgbe_info_pba - get Product Board Assemby
 * @adapter: adapter instance
 * @ctx: context providing buffer and additional information for the request
 *
 * Format PBA string and put it into the context buffer
 * in the Big Endian format.
 */
static void ixgbe_info_pba(struct ixgbe_adapter *adapter,
			   struct ixgbe_info_ctx *ctx)
{
	struct ixgbe_hw *hw = &adapter->hw;
	u32 status;

	status = ixgbe_read_pba_string(hw, (u8 *)ctx->buf, sizeof(ctx->buf));
	if (status)
		/* We failed to locate the PBA, so just skip this entry */
		dev_dbg(ixgbe_pf_to_dev(adapter),
			"Failed to read Product Board Assembly string, status %d\n",
			status);
}

/**
 * ixgbe_info_fw_mgmt - get Firmware version
 * @adapter: adapter instance
 * @ctx: context providing buffer and additional information for the request
 *
 * Format Firmware major, minor and patch versions string
 * and put it into the context buffer.
 */
static void ixgbe_info_fw_mgmt(struct ixgbe_adapter *adapter,
			       struct ixgbe_info_ctx *ctx)
{
	struct ixgbe_hw *hw = &adapter->hw;

	snprintf(ctx->buf, sizeof(ctx->buf), "%u.%u.%u",
		 hw->fw_maj_ver, hw->fw_min_ver, hw->fw_patch);
}

/**
 * ixgbe_info_fw_api - get Firmware API version
 * @adapter: adapter instance
 * @ctx: context providing buffer and additional information for the request
 *
 * Format Firmware API major, minor and patch versions string
 * and put it into the context buffer.
 */
static void ixgbe_info_fw_api(struct ixgbe_adapter *adapter,
			      struct ixgbe_info_ctx *ctx)
{
	struct ixgbe_hw *hw = &adapter->hw;

	snprintf(ctx->buf, sizeof(ctx->buf), "%u.%u.%u",
		 hw->api_maj_ver, hw->api_min_ver, hw->api_patch);
}

/**
 * ixgbe_info_fw_build - get Firmware build
 * @adapter: adapter instance
 * @ctx: context providing buffer and additional information for the request
 *
 * Format Firmware build string and put it into
 * the context buffer.
 */
static void ixgbe_info_fw_build(struct ixgbe_adapter *adapter,
				struct ixgbe_info_ctx *ctx)
{
	struct ixgbe_hw *hw = &adapter->hw;

	snprintf(ctx->buf, sizeof(ctx->buf), "0x%08x", hw->fw_build);
}

/**
 * ixgbe_info_fw_srev - get Firmware Serial Revision
 * @adapter: adapter instance
 * @ctx: context providing buffer and additional information for the request
 *
 * Format Firmware serial revision string and put it
 * into the context buffer.
 */
static void ixgbe_info_fw_srev(struct ixgbe_adapter *adapter,
			       struct ixgbe_info_ctx *ctx)
{
	struct ixgbe_nvm_info *nvm = &adapter->hw.flash.nvm;

	snprintf(ctx->buf, sizeof(ctx->buf), "%u", nvm->srev);
}

/**
 * ixgbe_info_pending_fw_srev - get pending Firmware Serial Revision
 * @adapter: adapter instance
 * @ctx: context providing buffer and additional information for the request
 *
 * Format Firmware serial revision string and
 * put it into the context buffer if there is a pending update.
 */
static void ixgbe_info_pending_fw_srev(struct ixgbe_adapter *adapter,
				       struct ixgbe_info_ctx *ctx)
{
	struct ixgbe_nvm_info *nvm = &ctx->pending_nvm;

	if (ctx->dev_caps.common_cap.nvm_update_pending_nvm)
		snprintf(ctx->buf, sizeof(ctx->buf), "%u", nvm->srev);
}

/**
 * ixgbe_info_orom_ver - get OROM version
 * @adapter: adapter instance
 * @ctx: context providing buffer and additional information for the request
 *
 * Format OROM major version, build and patch string and
 * put it into the context buffer.
 */
static void ixgbe_info_orom_ver(struct ixgbe_adapter *adapter,
				struct ixgbe_info_ctx *ctx)
{
	struct ixgbe_orom_info *orom = &adapter->hw.flash.orom;

	snprintf(ctx->buf, sizeof(ctx->buf), "%u.%u.%u",
		 orom->major, orom->build, orom->patch);
}

/**
 * ixgbe_info_pending_orom_ver - get pending OROM version
 * @adapter: adapter instance
 * @ctx: context providing buffer and additional information for the request
 *
 * Format OROM major version, build and patch string and
 * put it into the context buffer if there is a pending update.
 */
static void ixgbe_info_pending_orom_ver(struct ixgbe_adapter *adapter,
					struct ixgbe_info_ctx *ctx)
{
	struct ixgbe_orom_info *orom = &ctx->pending_orom;

	if (ctx->dev_caps.common_cap.nvm_update_pending_orom)
		snprintf(ctx->buf, sizeof(ctx->buf), "%u.%u.%u",
			 orom->major, orom->build, orom->patch);
}

/**
 * ixgbe_info_orom_srev - get OROM Serial Revision
 * @adapter: adapter instance
 * @ctx: context providing buffer and additional information for the request
 *
 * Format OROM string and put it into the context buffer.
 */
static void ixgbe_info_orom_srev(struct ixgbe_adapter *adapter,
				 struct ixgbe_info_ctx *ctx)
{
	struct ixgbe_orom_info *orom = &adapter->hw.flash.orom;

	snprintf(ctx->buf, sizeof(ctx->buf), "%u", orom->srev);
}

/**
 * ixgbe_info_pending_orom_srev - get pending OROM Serial Revision
 * @adapter: adapter instance
 * @ctx: context providing buffer and additional information for the request
 *
 * Format OROM serial revision string and put
 * it into the context buffer if there is a pending update.
 */
static void ixgbe_info_pending_orom_srev(struct ixgbe_adapter *adapter,
					 struct ixgbe_info_ctx *ctx)
{
	struct ixgbe_orom_info *orom = &ctx->pending_orom;

	if (ctx->dev_caps.common_cap.nvm_update_pending_orom)
		snprintf(ctx->buf, sizeof(ctx->buf), "%u", orom->srev);
}

/**
 * ixgbe_info_nvm_ver - get NVM version
 * @adapter: adapter instance
 * @ctx: context providing buffer and additional information for the request
 *
 * Format NVM major and minor versions string and put
 * it into the context buffer.
 */
static void ixgbe_info_nvm_ver(struct ixgbe_adapter *adapter,
			       struct ixgbe_info_ctx *ctx)
{
	struct ixgbe_nvm_info *nvm = &adapter->hw.flash.nvm;

	snprintf(ctx->buf, sizeof(ctx->buf), "%x.%02x", nvm->major, nvm->minor);
}

/**
 * ixgbe_info_pending_nvm_ver - get pending NVM version
 * @adapter: adapter instance
 * @ctx: context providing buffer and additional information for the request
 *
 * Format NVM major and minor versions string and put
 * it into the context buffer if there is a pending update.
 */
static void ixgbe_info_pending_nvm_ver(struct ixgbe_adapter *adapter,
				       struct ixgbe_info_ctx *ctx)
{
	struct ixgbe_nvm_info *nvm = &ctx->pending_nvm;

	if (ctx->dev_caps.common_cap.nvm_update_pending_nvm)
		snprintf(ctx->buf, sizeof(ctx->buf), "%x.%02x",
			 nvm->major, nvm->minor);
}

/**
 * ixgbe_info_eetrack - get NVM EEtrack ID
 * @adapter: adapter instance
 * @ctx: context providing buffer and additional information for the request
 *
 * Format NVM EEtrack ID string and put
 * it into the context buffer.
 */
static void ixgbe_info_eetrack(struct ixgbe_adapter *adapter,
			       struct ixgbe_info_ctx *ctx)
{
	struct ixgbe_nvm_info *nvm = &adapter->hw.flash.nvm;

	snprintf(ctx->buf, sizeof(ctx->buf), "0x%08x", nvm->eetrack);
}

/**
 * ixgbe_info_pending_eetrack - get pending NVM EEtrack ID
 * @adapter: adapter instance
 * @ctx: context providing buffer and additional information for the request
 *
 * Format NVM EEtrack ID and put it into
 * the context buffer if there is a pending update.
 */
static void ixgbe_info_pending_eetrack(struct ixgbe_adapter *adapter,
				       struct ixgbe_info_ctx *ctx)
{
	struct ixgbe_nvm_info *nvm = &ctx->pending_nvm;

	if (ctx->dev_caps.common_cap.nvm_update_pending_nvm)
		snprintf(ctx->buf, sizeof(ctx->buf), "0x%08x", nvm->eetrack);
}

/**
 * ixgbe_info_netlist_ver - get Netlist version
 * @adapter: adapter instance
 * @ctx: context providing buffer and additional information for the request
 *
 * Format Netlist major version, minor version,
 * type, revision and customer version string and put it
 * into the context buffer.
 */
static void ixgbe_info_netlist_ver(struct ixgbe_adapter *adapter,
				   struct ixgbe_info_ctx *ctx)
{
	struct ixgbe_netlist_info *netlist = &adapter->hw.flash.netlist;

	/* The netlist version fields are BCD formatted */
	snprintf(ctx->buf, sizeof(ctx->buf), "%x.%x.%x-%x.%x.%x",
		 netlist->major, netlist->minor,
		 netlist->type >> 16, netlist->type & 0xFFFF,
		 netlist->rev, netlist->cust_ver);
}

/**
 * ixgbe_info_netlist_build - get Netlist build
 * @adapter: adapter instance
 * @ctx: context providing buffer and additional information for the request
 *
 * Format Netlist hash (build) and puts it into the context buffer.
 */
static void ixgbe_info_netlist_build(struct ixgbe_adapter *adapter,
				     struct ixgbe_info_ctx *ctx)
{
	struct ixgbe_netlist_info *netlist = &adapter->hw.flash.netlist;

	snprintf(ctx->buf, sizeof(ctx->buf), "0x%08x", netlist->hash);
}

/**
 * ixgbe_info_pending_netlist_ver - get pending Netlist version
 * @adapter: adapter instance
 * @ctx: context providing buffer and additional information for the request
 *
 * Format Netlist major version, minor version,
 * type, revision and customer version string and put it
 * into the context buffer if there is a pending update.
 */
static void ixgbe_info_pending_netlist_ver(struct ixgbe_adapter *adapter,
					   struct ixgbe_info_ctx *ctx)
{
	struct ixgbe_netlist_info *netlist = &adapter->hw.flash.netlist;

	/* The netlist version fields are BCD formatted */
	if (ctx->dev_caps.common_cap.nvm_update_pending_netlist)
		snprintf(ctx->buf, sizeof(ctx->buf), "%x.%x.%x-%x.%x.%x",
			 netlist->major, netlist->minor,
			 netlist->type >> 16, netlist->type & 0xFFFF,
			 netlist->rev, netlist->cust_ver);
}

/**
 * ixgbe_info_pending_netlist_build - get Netlist build
 * @adapter: adapter instance
 * @ctx: context providing buffer and additional information for the request
 *
 * Format Netlist hash (build) and put it into the context buffer
 * if there is a pending update.
 */
static void ixgbe_info_pending_netlist_build(struct ixgbe_adapter *adapter,
					     struct ixgbe_info_ctx *ctx)
{
	struct ixgbe_netlist_info *netlist = &ctx->pending_netlist;

	if (ctx->dev_caps.common_cap.nvm_update_pending_netlist)
		snprintf(ctx->buf, sizeof(ctx->buf), "0x%08x", netlist->hash);
}

#define fixed(key, getter) { IXGBE_VERSION_FIXED, key, getter, NULL }
#define running(key, getter) { IXGBE_VERSION_RUNNING, key, getter, NULL }
#define stored(key, getter, fallback) \
	{ IXGBE_VERSION_STORED, key, getter, fallback }

enum ixgbe_version_type {
	IXGBE_VERSION_FIXED,
	IXGBE_VERSION_RUNNING,
	IXGBE_VERSION_STORED,
};

static const struct ixgbe_devlink_version {
	enum ixgbe_version_type type;
	const char *key;
	void (*getter)(struct ixgbe_adapter *adapter,
		       struct ixgbe_info_ctx *ctx);
	void (*fallback)(struct ixgbe_adapter *adapter,
			 struct ixgbe_info_ctx *ctx);
} ixgbe_devlink_versions[] = {
	fixed(DEVLINK_INFO_VERSION_GENERIC_BOARD_ID,
	      ixgbe_info_pba),
	running(DEVLINK_INFO_VERSION_GENERIC_FW_MGMT,
		ixgbe_info_fw_mgmt),
	running("fw.mgmt.api",
		ixgbe_info_fw_api),
	running("fw.mgmt.build",
		ixgbe_info_fw_build),

	running("fw.mgmt.srev", ixgbe_info_fw_srev),
	stored("fw.mgmt.srev",
	       ixgbe_info_pending_fw_srev, ixgbe_info_fw_srev),
	running(DEVLINK_INFO_VERSION_GENERIC_FW_UNDI, ixgbe_info_orom_ver),
	stored(DEVLINK_INFO_VERSION_GENERIC_FW_UNDI,
	       ixgbe_info_pending_orom_ver, ixgbe_info_orom_ver),
	running("fw.undi.srev", ixgbe_info_orom_srev),
	stored("fw.undi.srev",
	       ixgbe_info_pending_orom_srev, ixgbe_info_orom_srev),
	running("fw.psid.api", ixgbe_info_nvm_ver),
	stored("fw.psid.api",
	       ixgbe_info_pending_nvm_ver, ixgbe_info_nvm_ver),
	running(DEVLINK_INFO_VERSION_GENERIC_FW_BUNDLE_ID, ixgbe_info_eetrack),
	stored(DEVLINK_INFO_VERSION_GENERIC_FW_BUNDLE_ID,
	       ixgbe_info_pending_eetrack, ixgbe_info_eetrack),
	running("fw.netlist", ixgbe_info_netlist_ver),
	stored("fw.netlist",
	       ixgbe_info_pending_netlist_ver, ixgbe_info_netlist_ver),
	running("fw.netlist.build", ixgbe_info_netlist_build),
	stored("fw.netlist.build",
	       ixgbe_info_pending_netlist_build, ixgbe_info_netlist_build),
};

/**
 * ixgbe_devlink_info_get - Devlink handler for retrieving device info
 * @devlink: Devlink instance structure
 * @req: The devlink info request
 * @extack: Extended netdev ack structure
 *
 * This function serves as a callback for the devlink .info_get operation,
 * providing detailed information about the ixgbe network adapter. It
 * discovers device capabilities, retrieves version information, and reports
 * it through the devlink interface. The function handles errors in
 * retrieving data and sets appropriate error messages in the extack
 * structure.
 *
 * Return: 0 on success, or an error code on failure.
 */
static int ixgbe_devlink_info_get(struct devlink *devlink,
				  struct devlink_info_req *req,
				  struct netlink_ext_ack *extack)
{
	struct ixgbe_adapter *adapter;
	struct ixgbe_info_ctx *ctx;
	struct ixgbe_hw *hw;
	struct device *dev;
	s32 status;
	size_t i;
	int err;

	adapter = *(struct ixgbe_adapter **)devlink_priv(devlink);
	dev = ixgbe_pf_to_dev(adapter);
	hw = &adapter->hw;

	ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	/* discover capabilities first */
	status = ixgbe_discover_dev_caps(hw, &ctx->dev_caps);
	if (status) {
		dev_dbg(dev,
			"Failed to discover device capabilities, status %d aq_err %d\n",
			status,	hw->aci.last_status);
		NL_SET_ERR_MSG_MOD(extack,
				   "Unable to discover device capabilities");
		err = -EIO;
		goto out_free_ctx;
	}
	if (ctx->dev_caps.common_cap.nvm_update_pending_orom) {
		status = ixgbe_get_inactive_orom_ver(hw, &ctx->pending_orom);
		if (status) {
			dev_dbg(dev,
				"Unable to read inactive Option ROM version data, status %d aq_err %d\n",
				status, hw->aci.last_status);

			/* disable display of pending Option ROM */
			ctx->dev_caps.common_cap.nvm_update_pending_orom =
				false;
		}
	}

	if (ctx->dev_caps.common_cap.nvm_update_pending_nvm) {
		status = ixgbe_get_inactive_nvm_ver(hw, &ctx->pending_nvm);
		if (status) {
			dev_dbg(dev,
				"Unable to read inactive NVM version data, status %d aq_err %d\n",
				status, hw->aci.last_status);

			/* disable display of pending Option ROM */
			ctx->dev_caps.common_cap.nvm_update_pending_nvm = false;
		}
	}

	if (ctx->dev_caps.common_cap.nvm_update_pending_netlist) {
		status = ixgbe_get_inactive_netlist_ver(hw,
							&ctx->pending_netlist);
		if (status) {
			dev_dbg(dev,
				"Unable to read inactive Netlist version data, status %d aq_err %d\n",
				status, hw->aci.last_status);

		/* disable display of pending Option ROM */
		ctx->dev_caps.common_cap.nvm_update_pending_netlist =
			false;
		}
	}

#ifdef HAVE_DEVLINK_INFO_DRIVER_NAME_PUT
	err = devlink_info_driver_name_put(req, KBUILD_MODNAME);
	if (err) {
		NL_SET_ERR_MSG_MOD(extack, "Unable to set driver name");
		goto out_free_ctx;
	}
#endif /* HAVE_DEVLINK_INFO_DRIVER_NAME_PUT */

	ixgbe_info_get_dsn(adapter, ctx);

	err = devlink_info_serial_number_put(req, ctx->buf);
	if (err) {
		NL_SET_ERR_MSG_MOD(extack, "Unable to set serial number");
		goto out_free_ctx;
	}

	for (i = 0; i < ARRAY_SIZE(ixgbe_devlink_versions); i++) {
		enum ixgbe_version_type type = ixgbe_devlink_versions[i].type;
		const char *key = ixgbe_devlink_versions[i].key;

		memset(ctx->buf, 0, sizeof(ctx->buf));

		ixgbe_devlink_versions[i].getter(adapter, ctx);

		/* If the default getter doesn't report a version, use the
		 * fallback function. This is primarily useful in the case of
		 * "stored" versions that want to report the same value as the
		 * running version in the normal case of no pending update.
		 */
		if (ctx->buf[0] == '\0' && ixgbe_devlink_versions[i].fallback)
			ixgbe_devlink_versions[i].fallback(adapter, ctx);

		/* Do not report missing versions */
		if (ctx->buf[0] == '\0')
			continue;

		switch (type) {
		case IXGBE_VERSION_FIXED:
			err = devlink_info_version_fixed_put(req, key,
							     ctx->buf);
			if (err) {
				NL_SET_ERR_MSG_MOD(extack,
						   "Unable to set fixed version");
				goto out_free_ctx;
			}
			break;
		case IXGBE_VERSION_RUNNING:
			err = devlink_info_version_running_put(req, key,
							       ctx->buf);
			if (err) {
				NL_SET_ERR_MSG_MOD(extack,
						   "Unable to set running version");
				goto out_free_ctx;
			}
			break;
		case IXGBE_VERSION_STORED:
			err = devlink_info_version_stored_put(req, key,
							      ctx->buf);
			if (err) {
				NL_SET_ERR_MSG_MOD(extack,
						   "Unable to set stored version");
				goto out_free_ctx;
			}
			break;
		}
	}
out_free_ctx:
	kfree(ctx);
	return err;
}
#endif /* HAVE_DEVLINK_INFO_GET */

#ifdef HAVE_DEVLINK_PARAMS
enum ixgbe_devlink_param_id {
	IXGBE_DEVLINK_PARAM_ID_BASE = DEVLINK_PARAM_GENERIC_ID_MAX,
	IXGBE_DEVLINK_PARAM_ID_FW_MGMT_MINSREV,
	IXGBE_DEVLINK_PARAM_ID_FW_UNDI_MINSREV,
};

/**
 * ixgbe_devlink_minsrev_get - Get the current minimum security revision
 * @devlink: pointer to the devlink instance
 * @id: the parameter ID to get
 * @ctx: context to return the parameter value
 *
 * Returns: zero on success, or an error code on failure.
 */
static int ixgbe_devlink_minsrev_get(struct devlink *devlink, u32 id,
				     struct devlink_param_gset_ctx *ctx)
{
	struct ixgbe_minsrev_info minsrevs = {};
	struct ixgbe_adapter *adapter;
	struct device *dev;
	s32 status;

	adapter = *(struct ixgbe_adapter **)devlink_priv(devlink);
	dev = ixgbe_pf_to_dev(adapter);

	if (id != IXGBE_DEVLINK_PARAM_ID_FW_MGMT_MINSREV &&
	    id != IXGBE_DEVLINK_PARAM_ID_FW_UNDI_MINSREV)
		return -EINVAL;

	status = ixgbe_get_nvm_minsrevs(&adapter->hw, &minsrevs);

	if (status == IXGBE_ERR_ACI_ERROR) {
		ixgbe_release_nvm(&adapter->hw);
		status = ixgbe_get_nvm_minsrevs(&adapter->hw, &minsrevs);
	}

	if (status) {
		dev_warn(dev,
			 "Failed to read minimum security revision data from flash\n");
		return -EIO;
	}

	/* We report zero if the device has not yet had a valid minimum
	 * security revision programmed for the associated module. This makes
	 * sense because it is not possible to have a security revision of
	 * less than zero. Thus, all images will be able to load if the
	 * minimum security revision is zero, the same as the case where the
	 * minimum value is indicated as invalid.
	 */

	switch (id) {
	case IXGBE_DEVLINK_PARAM_ID_FW_MGMT_MINSREV:
		if (minsrevs.nvm_valid)
			ctx->val.vu32 = minsrevs.nvm;
		else
			ctx->val.vu32 = 0;
		break;
	case IXGBE_DEVLINK_PARAM_ID_FW_UNDI_MINSREV:
		if (minsrevs.orom_valid)
			ctx->val.vu32 = minsrevs.orom;
		else
			ctx->val.vu32 = 0;
		break;
	}

	return 0;
}

#ifdef HAVE_DEVLINK_PARAMS_SET_EXTACK
/**
 * ixgbe_devlink_minsrev_set - Set the minimum security revision
 * @devlink: pointer to the devlink instance
 * @id: the parameter ID to set
 * @ctx: context to return the parameter value
 * @extack: netlink extended ACK structure
 *
 * Set the minimum security revision value for fw.mgmt or fw.undi. The kernel
 * calls the validate handler before calling this, so we do not need to
 * duplicate those checks here.
 *
 * Returns: zero on success, or an error code on failure.
 */
static int ixgbe_devlink_minsrev_set(struct devlink *devlink, u32 id,
				     struct devlink_param_gset_ctx *ctx,
				     struct netlink_ext_ack *extack)
#else
static int ixgbe_devlink_minsrev_set(struct devlink *devlink, u32 id,
				     struct devlink_param_gset_ctx *ctx)
#endif
{
	struct ixgbe_minsrev_info minsrevs = {};
	struct ixgbe_adapter *adapter;
	struct device *dev;
	s32 status;

	adapter = *(struct ixgbe_adapter **)devlink_priv(devlink);
	dev = ixgbe_pf_to_dev(adapter);

	switch (id) {
	case IXGBE_DEVLINK_PARAM_ID_FW_MGMT_MINSREV:
		minsrevs.nvm_valid = true;
		minsrevs.nvm = ctx->val.vu32;
		break;
	case IXGBE_DEVLINK_PARAM_ID_FW_UNDI_MINSREV:
		minsrevs.orom_valid = true;
		minsrevs.orom = ctx->val.vu32;
		break;
	default:
		return -EINVAL;
	}

	status = ixgbe_update_nvm_minsrevs(&adapter->hw, &minsrevs);
	if (status) {
		dev_warn(dev,
			 "Failed to update minimum security revision data\n");
		return -EIO;
	}

	return 0;
}

/**
 * ixgbe_devlink_minsrev_validate - Validate a minimum security revision update
 * @devlink: unused pointer to devlink instance
 * @id: the parameter ID to validate
 * @val: value to validate
 * @extack: netlink extended ACK structure
 *
 * Check that a proposed update to a minimum security revision field is valid.
 * Each minimum security revision can only be increased, not decreased.
 * Additionally, we verify that the value is never set higher than the
 * security revision of the active flash component.
 *
 * Returns: zero if the value is valid, -ERANGE if it is out of range, and
 * -EINVAL if this function is called with the wrong ID.
 */
static int ixgbe_devlink_minsrev_validate(struct devlink *devlink, u32 id,
					  union devlink_param_value val,
					  struct netlink_ext_ack *extack)
{
	struct ixgbe_minsrev_info minsrevs = {};
	struct ixgbe_adapter *adapter;
	struct device *dev;
	s32 status;

	adapter = *(struct ixgbe_adapter **)devlink_priv(devlink);
	dev = ixgbe_pf_to_dev(adapter);

	if (id != IXGBE_DEVLINK_PARAM_ID_FW_MGMT_MINSREV &&
	    id != IXGBE_DEVLINK_PARAM_ID_FW_UNDI_MINSREV)
		return -EINVAL;

	status = ixgbe_get_nvm_minsrevs(&adapter->hw, &minsrevs);

	if (status == IXGBE_ERR_ACI_ERROR) {
		ixgbe_release_nvm(&adapter->hw);
		status = ixgbe_get_nvm_minsrevs(&adapter->hw, &minsrevs);
	}
	if (status) {
		NL_SET_ERR_MSG_MOD(extack,
				   "Failed to read minimum security revision data from flash");
		return -EIO;
	}

	switch (id) {
	case IXGBE_DEVLINK_PARAM_ID_FW_MGMT_MINSREV:
		if (val.vu32 > adapter->hw.flash.nvm.srev) {
			NL_SET_ERR_MSG_MOD(extack,
					   "Cannot update fw.mgmt minimum security revision higher than the currently running firmware");
			dev_dbg(dev,
				"Attempted to set fw.mgmt.minsrev to %u, but running firmware has srev %u\n",
				val.vu32, adapter->hw.flash.nvm.srev);
			return -EPERM;
		}

		if (minsrevs.nvm_valid && val.vu32 < minsrevs.nvm) {
			NL_SET_ERR_MSG_MOD(extack,
					   "Cannot lower the minimum security revision for fw.mgmt flash section");
			dev_dbg(dev,
				"Attempted to set fw.mgmt.minsrev to %u, but current minsrev is %u\n",
				val.vu32, minsrevs.nvm);
			return -EPERM;
		}
		break;
	case IXGBE_DEVLINK_PARAM_ID_FW_UNDI_MINSREV:
		if (val.vu32 > adapter->hw.flash.orom.srev) {
			NL_SET_ERR_MSG_MOD(extack,
					   "Cannot update fw.undi minimum security revision higher than the currently running firmware");
			dev_dbg(dev,
				"Attempted to set fw.undi.minsrev to %u, but running firmware has srev %u\n",
				val.vu32, adapter->hw.flash.orom.srev);
			return -EPERM;
		}

		if (minsrevs.orom_valid && val.vu32 < minsrevs.orom) {
			NL_SET_ERR_MSG_MOD(extack,
					   "Cannot lower the minimum security revision for fw.undi flash section");
			dev_dbg(dev,
				"Attempted to set fw.undi.minsrev to %u, but current minsrev is %u\n",
				val.vu32, minsrevs.orom);
			return -EPERM;
		}
		break;
	}

	return 0;
}

/* devlink parameters for the driver */
static const struct devlink_param ixgbe_devlink_params[] = {
	DEVLINK_PARAM_DRIVER(IXGBE_DEVLINK_PARAM_ID_FW_MGMT_MINSREV,
			     "fw.mgmt.minsrev",
			     DEVLINK_PARAM_TYPE_U32,
			     BIT(DEVLINK_PARAM_CMODE_PERMANENT),
			     ixgbe_devlink_minsrev_get,
			     ixgbe_devlink_minsrev_set,
			     ixgbe_devlink_minsrev_validate),
	DEVLINK_PARAM_DRIVER(IXGBE_DEVLINK_PARAM_ID_FW_UNDI_MINSREV,
			     "fw.undi.minsrev",
			     DEVLINK_PARAM_TYPE_U32,
			     BIT(DEVLINK_PARAM_CMODE_PERMANENT),
			     ixgbe_devlink_minsrev_get,
			     ixgbe_devlink_minsrev_set,
			     ixgbe_devlink_minsrev_validate),
};
#endif /* HAVE_DEVLINK_PARAMS */

#ifdef HAVE_DEVLINK_FLASH_UPDATE
#ifdef HAVE_DEVLINK_FLASH_UPDATE_BEGIN_END_NOTIFY
/**
 * ixgbe_devlink_flash_update_notify - Compatibility for begin/end notify
 * @devlink: pointer to the devlink instance for this device
 * @params: flash update parameters
 * @extack: netlink extended ACK message structure
 *
 * Compatibility wrapper which handles calling
 * devlink_flash_update_begin_notify and devlink_flash_update_end_notify when
 * the kernel does not do this for us.
 *
 * Return: 0 on success or negative error code on failure.
 */
static int ixgbe_devlink_flash_update_notify(struct devlink *devlink,
					     struct devlink_flash_update_params
					     *params,
					     struct netlink_ext_ack *extack)
{
	int err;

	devlink_flash_update_begin_notify(devlink);
	err = ixgbe_flash_pldm_image(devlink, params, extack);
	devlink_flash_update_end_notify(devlink);

	return err;
}
#endif /* HAVE_DEVLINK_FLASH_UPDATE_BEGIN_END_NOTIFY */

#ifndef HAVE_DEVLINK_FLASH_UPDATE_PARAMS
/**
 * ixgbe_devlink_flash_update_params - Compatibility for params argument
 * @devlink: pointer to the devlink instance for this device
 * @file_name: the file name to request the firmware from
 * @component: the flash component to update
 * @extack: netlink extended ACK message structure
 *
 * Compatibility wrapper which handles creating the flash update parameters
 * structure for kernels which do not have this structure defined yet.
 *
 * Return: 0 on success or negative error code on failure.
 */
static int ixgbe_devlink_flash_update_params(struct devlink *devlink,
					     const char *file_name,
					     const char *component,
					     struct netlink_ext_ack *extack)
{
	struct devlink_flash_update_params params = {};
	struct ixgbe_adapter *adapter;
	struct device *dev;
	int ret;

	adapter = *(struct ixgbe_adapter **)devlink_priv(devlink);
	dev = ixgbe_pf_to_dev(adapter);

	/* individual component update is not yet supported, and older kernels
	 * did not check this for us.
	 */
	if (component)
		return -EOPNOTSUPP;

	params.file_name = file_name;

#ifdef HAVE_DEVLINK_FLASH_UPDATE_BEGIN_END_NOTIFY
	ret = ixgbe_devlink_flash_update_notify(devlink, &params, extack);

	if (ret)
		dev_dbg(dev,
			"ixgbe_devlink_flash_update_notify() returned %d\n",
			ret);
#else
	ret = ixgbe_flash_pldm_image(devlink, &params, extack);

	if (ret)
		dev_dbg(dev, "ixgbe_flash_pldm_image() returned %d\n", ret);
#endif
	return ret;
}
#endif /* !HAVE_DEVLINK_FLASH_UPDATE_PARAMS */
#endif /* HAVE_DEVLINK_FLASH_UPDATE */

#ifdef HAVE_DEVLINK_RELOAD_ACTION_AND_LIMIT
/**
 * ixgbe_devlink_reload_empr_start - Start EMP reset to activate new firmware
 * @devlink: pointer to the devlink instance to reload
 * @netns_change: if true, the network namespace is changing
 * @action: the action to perform. Must be DEVLINK_RELOAD_ACTION_FW_ACTIVATE
 * @limit: limits on what reload should do, such as not resetting
 * @extack: netlink extended ACK structure
 *
 * Allow user to activate new Embedded Management Processor firmware by
 * issuing device specific EMP reset. Called in response to
 * a DEVLINK_CMD_RELOAD with the DEVLINK_RELOAD_ACTION_FW_ACTIVATE.
 *
 * Note that teardown and rebuild of the driver state happens automatically as
 * part of an interrupt and watchdog task. This is because all physical
 * functions on the device must be able to reset when an EMP reset occurs from
 * any source.
 *
 * Return: the exit code of the operation.
 */
static int ixgbe_devlink_reload_empr_start(struct devlink *devlink,
					   bool netns_change,
					   enum devlink_reload_action action,
					   enum devlink_reload_limit limit,
					   struct netlink_ext_ack *extack)
{
	struct ixgbe_adapter *adapter;
	struct ixgbe_hw *hw;
	struct device *dev;
	s32 status;
	u8 pending;
	int err;

	adapter = *(struct ixgbe_adapter **)devlink_priv(devlink);
	dev = ixgbe_pf_to_dev(adapter);
	hw = &adapter->hw;

	err = ixgbe_get_pending_updates(adapter, &pending, extack);
	if (err)
		return err;

	/* pending is a bitmask of which flash banks have a pending update,
	 * including the main NVM bank, the Option ROM bank, and the netlist
	 * bank. If any of these bits are set, then there is a pending update
	 * waiting to be activated.
	 */
	if (!pending) {
		NL_SET_ERR_MSG_MOD(extack, "No pending firmware update");
		return -ECANCELED;
	}

	if (adapter->fw_emp_reset_disabled) {
		NL_SET_ERR_MSG_MOD(extack,
				   "EMP reset is not available. To activate firmware, a reboot or power cycle is needed\n");
		return -ECANCELED;
	}

	dev_dbg(dev, "Issuing device EMP reset to activate firmware\n");

	status = ixgbe_aci_nvm_update_empr(hw);
	if (status) {
		dev_err(dev,
			"Failed to trigger EMP device reset to reload firmware, err %d\n",
			status);
		NL_SET_ERR_MSG_MOD(extack,
				   "Failed to trigger EMP device reset to reload firmware");
		return -EIO;
	}

	return 0;
}

/* Wait for 10 sec with 0.5 sec tic. EMPR takes no less than half of a sec */
#define IXGBE_DEVLINK_RELOAD_TICK_L	400000
#define IXGBE_DEVLINK_RELOAD_TICK_H	500000
#define IXGBE_DEVLINK_RELOAD_TIMEOUT	20

/**
 * ixgbe_devlink_reload_empr_finish - Complete EMP reset process
 * @devlink: Pointer to the devlink instance
 * @action: The action to perform
 * @limit: Limits on what reload should do
 * @actions_performed: Actions performed
 * @extack: Netlink extended ACK structure
 *
 * This function completes the EMP reset process for the ixgbe network
 * adapter. It updates the actions performed to indicate firmware activation
 * and waits for the device and driver to finish the reset and rebuild
 * process. The function currently includes a temporary 10-second wait to
 * ensure completion.
 *
 * Return: Always returns 0, what means success.
 */
static int ixgbe_devlink_reload_empr_finish(struct devlink *devlink,
					    enum devlink_reload_action action,
					    enum devlink_reload_limit limit,
					    u32 *actions_performed,
					    struct netlink_ext_ack *extack)
{
	struct ixgbe_adapter *adapter =
		*(struct ixgbe_adapter **)devlink_priv(devlink);
	struct ixgbe_hw *hw = &adapter->hw;
	int i = 0;
	u32 fwsm;

	do {
		if (i++ >= IXGBE_DEVLINK_RELOAD_TIMEOUT)
			return -ETIME;

		/* Just right away after triggering EMP reset the FWSM register
		 * may be not cleared yet, so begin the loop with the delay
		 * in order to not check the not updated register.
		 */
		usleep_range(IXGBE_DEVLINK_RELOAD_TICK_L,
			     IXGBE_DEVLINK_RELOAD_TICK_H);
		fwsm = IXGBE_READ_REG(hw, IXGBE_FWSM_BY_MAC(hw));

	} while (!(fwsm & IXGBE_FWSM_FW_VAL_BIT));

	*actions_performed = BIT(DEVLINK_RELOAD_ACTION_FW_ACTIVATE);

	return ixgbe_refresh_fw_version(adapter);
}
#endif /* HAVE_DEVLINK_RELOAD_ACTION_AND_LIMIT */


static const struct devlink_ops ixgbe_devlink_ops = {
#ifdef HAVE_DEVLINK_FLASH_UPDATE_PARAMS
	.supported_flash_update_params =
		DEVLINK_SUPPORT_FLASH_UPDATE_OVERWRITE_MASK,
#endif /* HAVE_DEVLINK_FLASH_UPDATE_PARAMS */
#ifdef HAVE_DEVLINK_RELOAD_ACTION_AND_LIMIT
	.reload_actions = BIT(DEVLINK_RELOAD_ACTION_FW_ACTIVATE),
	/* The ixgbe driver currently does not support driver reinit */
	.reload_down = ixgbe_devlink_reload_empr_start,
	.reload_up = ixgbe_devlink_reload_empr_finish,
#endif
#ifdef HAVE_DEVLINK_INFO_GET
	.info_get = ixgbe_devlink_info_get,
#endif /* HAVE_DEVLINK_INFO_GET */

#ifdef HAVE_DEVLINK_FLASH_UPDATE
#if !defined(HAVE_DEVLINK_FLASH_UPDATE_PARAMS)
	.flash_update = ixgbe_devlink_flash_update_params,
#elif defined(HAVE_DEVLINK_FLASH_UPDATE_BEGIN_END_NOTIFY)
	.flash_update = ixgbe_devlink_flash_update_notify,
#else
	.flash_update = ixgbe_flash_pldm_image,
#endif
#endif /* HAVE_DEVLINK_FLASH_UPDATE */
};

/**
 * ixgbe_allocate_devlink - Allocate devlink and return adapter pointer
 * @adapter: pointer to the device adapter structure
 *
 * Allocate a devlink instance for this device and return the private area as
 * the pointer to the adapter structure.
 *
 * Return: the pointer to adapter structure is returned
 */
struct ixgbe_adapter **ixgbe_allocate_devlink(struct ixgbe_adapter *adapter)
{
	struct device *dev = ixgbe_pf_to_dev(adapter);
	struct ixgbe_adapter **padapter;
	struct devlink *devlink;

	devlink = devlink_alloc(&ixgbe_devlink_ops,
				sizeof(struct ixgbe_adapter *), dev);
	if (!devlink)
		return NULL;
#ifdef HAVE_DEVLINK_REGISTER_SETS_DEV
	/* On older kernels, eg. 4.12,
	 * devlink->dev is not set during devlink_alloc.
	 * Assign it here just to be sure.
	 */
	devlink->dev = dev;
#endif /* HAVE_DEVLINK_REGISTER_SETS_DEV */

	padapter = (struct ixgbe_adapter **)devlink_priv(devlink);
	/* The adapter pointer is assigned to the indicated place in devlink */
	*padapter = adapter;
	/* Add pointer to devlink to the adapter structure */
	adapter->devlink = devlink;

	return padapter;
}

/**
 * ixgbe_devlink_register - Register devlink interface for this adapter
 * @adapter: pointer to the device adapter structure
 *
 * Register the devlink instance associated with this physical function.
 */
void ixgbe_devlink_register(struct ixgbe_adapter *adapter)
{
	struct devlink *devlink = adapter->devlink;

#ifdef HAVE_DEVLINK_SET_FEATURES
	devlink_set_features(devlink, DEVLINK_F_RELOAD);
#endif /* HAVE_DEVLINK_SET_FEATURES */
#ifdef HAVE_DEVLINK_REGISTER_SETS_DEV
	devlink_register(devlink, ixgbe_pf_to_dev(adapter));
#else
	devlink_register(devlink);
#endif

#ifdef HAVE_DEVLINK_RELOAD_ACTION_AND_LIMIT
#ifndef HAVE_DEVLINK_SET_FEATURES
#ifdef HAVE_DEVLINK_RELOAD_ENABLE_DISABLE
	devlink_reload_enable(devlink);
#endif /* HAVE_DEVLINK_RELOAD_ENABLE_DISABLE */
#endif /* !HAVE_DEVLINK_SET_FEATURES */
#endif /* HAVE_DEVLINK_RELOAD_ACTION_AND_LIMIT */
}

/**
 * ixgbe_devlink_unregister - Unregister devlink resources for this adapter.
 * @adapter: pointer to the device adapter structure
 *
 * Releases resources used by devlink and cleans up associated memory.
 */
void ixgbe_devlink_unregister(struct ixgbe_adapter *adapter)
{
	struct devlink *devlink = adapter->devlink;

#ifdef HAVE_DEVLINK_RELOAD_ACTION_AND_LIMIT
#ifndef HAVE_DEVLINK_SET_FEATURES
#ifdef HAVE_DEVLINK_RELOAD_ENABLE_DISABLE
	devlink_reload_disable(devlink);
#endif /* HAVE_DEVLINK_RELOAD_ENABLE_DISABLE */
#endif /* !HAVE_DEVLINK_SET_FEATURES */
#endif /* HAVE_DEVLINK_RELOAD_ACTION_AND_LIMIT */

	devlink_unregister(devlink);
}

/**
 * ixgbe_devlink_register_params - Register devlink parameters
 * @adapter: pointer to the device adapter structure
 *
 * Registers the parameters associated with this adapter.
 *
 * Return: the exit code of the operation.
 */
int ixgbe_devlink_register_params(struct ixgbe_adapter *adapter)
{
#ifdef HAVE_DEVLINK_PARAMS
	struct device *dev = ixgbe_pf_to_dev(adapter);
	struct devlink *devlink = adapter->devlink;
	int err;

	err = devlink_params_register(devlink, ixgbe_devlink_params,
				      ARRAY_SIZE(ixgbe_devlink_params));
	if (err) {
		dev_err(dev,
			"devlink params registration failed, err %d\n",
			err);
		return err;
	}

#ifdef HAVE_DEVLINK_PARAMS_PUBLISH
	devlink_params_publish(devlink);
#endif /* HAVE_DEVLINK_PARAMS_PUBLISH */

#endif /* HAVE_DEVLINK_PARAMS */
	return 0;
}

/**
 * ixgbe_devlink_unregister_params - Unregister devlink parameters
 * @adapter: pointer to the device adapter structure
 *
 * Removes the main devlink parameters associated with this adapter.
 */
void ixgbe_devlink_unregister_params(struct ixgbe_adapter *adapter)
{
#ifdef HAVE_DEVLINK_PARAMS
	struct devlink *devlink = adapter->devlink;

#ifdef HAVE_DEVLINK_PARAMS_PUBLISH
	devlink_params_unpublish(devlink);
#endif /* HAVE_DEVLINK_PARAMS_PUBLISH */

	devlink_params_unregister(devlink, ixgbe_devlink_params,
				  ARRAY_SIZE(ixgbe_devlink_params));
#endif /* HAVE_DEVLINK_PARAMS */
}

/**
 * ixgbe_devlink_register_port - Register a devlink port for the adapter
 * @adapter: pointer to the device adapter structure
 *
 * Create and register a devlink_port for this adapter.
 *
 * Return: zero on success or an error code on failure.
 */
int ixgbe_devlink_register_port(struct ixgbe_adapter *adapter)
{
	struct devlink_port *devlink_port = &adapter->devlink_port;
	struct device *dev = ixgbe_pf_to_dev(adapter);
	struct devlink *devlink = adapter->devlink;
	struct devlink_port_attrs attrs = {};
	int err;

	attrs.flavour = DEVLINK_PORT_FLAVOUR_PHYSICAL;
	attrs.phys.port_number = adapter->hw.bus.func;

	devlink_port_attrs_set(devlink_port, &attrs);

	err = devlink_port_register(devlink, devlink_port, 0);
	if (err) {
		dev_err(dev,
			"devlink port registration failed, err %d\n",
			err);
	}

	return err;
}

/**
 * ixgbe_devlink_unregister_port - Unregister the devlink port
 * @adapter: pointer to the device adapter structure
 *
 * Unregisters the devlink_port structure associated with this adapter.
 */
void ixgbe_devlink_unregister_port(struct ixgbe_adapter *adapter)
{
	struct devlink_port *devlink_port = &adapter->devlink_port;

	devlink_port_unregister(devlink_port);
}

#ifdef HAVE_DEVLINK_REGIONS
#define IXGBE_ACI_MAX_BUF_LEN 4096

#ifdef HAVE_DEVLINK_REGION_OPS_SNAPSHOT
#define IXGBE_DEVLINK_READ_BLK_SIZE (1024 * 1024)

#ifdef HAVE_DEVLINK_REGION_OPS_SNAPSHOT_OPS

/**
 * ixgbe_devlink_nvm_snapshot - Capture a snapshot of the NVM flash contents
 * @devlink: the devlink instance
 * @ops: the devlink region being snapshotted
 * @extack: extended ACK response structure
 * @data: on exit points to snapshot data buffer
 *
 * This function is called in response to the DEVLINK_CMD_REGION_TRIGGER for
 * the nvm-flash devlink region. It captures a snapshot of the full NVM flash
 * contents, including both banks of flash. This snapshot can later be viewed
 * via the devlink-region interface.
 *
 * It captures the flash using the FLASH_ONLY bit set when reading via
 * firmware, so it does not read the current Shadow RAM contents. For that,
 * use the shadow-ram region.
 *
 * @returns zero on success, and updates the data pointer. Returns a non-zero
 * error code on failure.
 */
#endif /* HAVE_DEVLINK_REGION_OPS_SNAPSHOT_OPS */
static int ixgbe_devlink_nvm_snapshot(struct devlink *devlink,
#ifdef HAVE_DEVLINK_REGION_OPS_SNAPSHOT_OPS
				      const struct devlink_region_ops
					 __always_unused *ops,
#endif /* HAVE_DEVLINK_REGION_OPS_SNAPSHOT_OPS */
				      struct netlink_ext_ack *extack, u8 **data)
{
	struct ixgbe_adapter *adapter;
	struct ixgbe_hw *hw;
	struct device *dev;
	u8 *nvm_data, *tmp;
	u32 nvm_size, left;
	s8 num_blks, i;
	s32 status;

	adapter = *(struct ixgbe_adapter **)devlink_priv(devlink);
	dev = ixgbe_pf_to_dev(adapter);
	hw = &adapter->hw;

	nvm_size = hw->flash.flash_size;
	nvm_data = vzalloc(nvm_size);
	if (!nvm_data)
		return -ENOMEM;

	num_blks = DIV_ROUND_UP(nvm_size, IXGBE_DEVLINK_READ_BLK_SIZE);
	tmp = nvm_data;
	left = nvm_size;

	/* some systems take longer to read the nvm than others which causes the
	 * fw to reclaim the nvm lock before the entire nvm has been read. fix
	 * this by breaking the reads of the nvm into smaller chunks that will
	 * probably not take as long. this has some overhead since we are
	 * increasing the number of AQ commands, but it should always work
	 */
	for (i = 0; i < num_blks; i++) {
		u32 read_sz = min_t(u32, IXGBE_DEVLINK_READ_BLK_SIZE, left);

		status = ixgbe_acquire_nvm(hw, IXGBE_RES_READ);
		if (status) {
			dev_dbg(dev,
				"ixgbe_acquire_nvm failed, err %d aq_err %d\n",
				status, hw->aci.last_status);
			NL_SET_ERR_MSG_MOD(extack,
					   "Failed to acquire NVM semaphore");
			vfree(nvm_data);
			return -EIO;
		}

		status = ixgbe_read_flat_nvm(hw,
					     i * IXGBE_DEVLINK_READ_BLK_SIZE,
					     &read_sz, tmp, false);
		if (status) {
			dev_dbg(dev,
				"ixgbe_read_flat_nvm failed after reading %u bytes, err %d aq_err %d\n",
				read_sz, status, hw->aci.last_status);
			NL_SET_ERR_MSG_MOD(extack,
					   "Failed to read RAM contents");
			ixgbe_release_nvm(hw);
			vfree(nvm_data);
			return -EIO;
		}

		ixgbe_release_nvm(hw);

		tmp += read_sz;
		left -= read_sz;
	}

	*data = nvm_data;

	return 0;
}

#ifdef HAVE_DEVLINK_REGION_OPS_SNAPSHOT_OPS
/**
 * ixgbe_devlink_sram_snapshot - Capture a snapshot of the Shadow RAM contents
 * @devlink: the devlink instance
 * @ops: the devlink region being snapshotted
 * @extack: extended ACK response structure
 * @data: on exit points to snapshot data buffer
 *
 * This function is called in response to the DEVLINK_CMD_REGION_TRIGGER for
 * the shadow-ram devlink region. It captures a snapshot of the shadow ram
 * contents. This snapshot can later be viewed via the devlink-region
 * interface.
 *
 * @returns zero on success, and updates the data pointer. Returns a non-zero
 * error code on failure.
 */
#endif /* HAVE_DEVLINK_REGION_OPS_SNAPSHOT_OPS */
static int ixgbe_devlink_sram_snapshot(struct devlink *devlink,
#ifdef HAVE_DEVLINK_REGION_OPS_SNAPSHOT_OPS
				       const struct devlink_region_ops
					__always_unused *ops,
#endif /* HAVE_DEVLINK_REGION_OPS_SNAPSHOT_OPS */
				       struct netlink_ext_ack *extack,
				       u8 **data)
{
	struct ixgbe_adapter *adapter;
	struct ixgbe_hw *hw;
	struct device *dev;
	u32 sram_size;
	u8 *sram_data;
	int status;

	adapter = *(struct ixgbe_adapter **)devlink_priv(devlink);
	dev = ixgbe_pf_to_dev(adapter);
	hw = &adapter->hw;

	sram_size = hw->flash.sr_words * 2u;
	sram_data = vzalloc(sram_size);
	if (!sram_data)
		return -ENOMEM;

	status = ixgbe_acquire_nvm(hw, IXGBE_RES_READ);
	if (status) {
		dev_dbg(dev,
			"ixgbe_acquire_nvm failed, err %d aq_err %d\n",
			status, hw->aci.last_status);
		NL_SET_ERR_MSG_MOD(extack, "Failed to acquire NVM semaphore");
		vfree(sram_data);
		return (int)status;
	}

	/* Read from the Shadow RAM, rather than directly from NVM */
	status = ixgbe_read_flat_nvm(hw, 0, &sram_size, sram_data, true);
	if (status) {
		dev_dbg(dev,
			"ixgbe_read_flat_nvm failed after reading %u bytes, err %d aq_err %d\n",
			sram_size, status, hw->aci.last_status);
		NL_SET_ERR_MSG_MOD(extack,
				   "Failed to read Shadow RAM contents");
		ixgbe_release_nvm(hw);
		vfree(sram_data);
		return (int)status;
	}

	ixgbe_release_nvm(hw);

	*data = sram_data;

	return 0;
}

#ifdef HAVE_DEVLINK_REGION_OPS_SNAPSHOT_OPS
/**
 * ixgbe_devlink_devcaps_snapshot - Capture snapshot of device capabilities
 * @devlink: the devlink instance
 * @ops: the devlink region being snapshotted
 * @extack: extended ACK response structure
 * @data: on exit points to snapshot data buffer
 *
 * This function is called in response to the DEVLINK_CMD_REGION_TRIGGER for
 * the device-caps devlink region. It captures a snapshot of the device
 * capabilities reported by firmware.
 *
 * @returns zero on success, and updates the data pointer. Returns a non-zero
 * error code on failure.
 */
#endif /* HAVE_DEVLINK_REGION_OPS_SNAPSHOT_OPS */
static int ixgbe_devlink_devcaps_snapshot(struct devlink *devlink,
#ifdef HAVE_DEVLINK_REGION_OPS_SNAPSHOT_OPS
					  const struct devlink_region_ops
						__always_unused *ops,
#endif /* HAVE_DEVLINK_REGION_OPS_SNAPSHOT_OPS */
					  struct netlink_ext_ack *extack,
					  u8 **data)
{
	struct ixgbe_adapter *adapter;
	struct ixgbe_hw *hw;
	struct device *dev;
	void *devcaps;
	s32 status;

	adapter = *(struct ixgbe_adapter **)devlink_priv(devlink);
	dev = ixgbe_pf_to_dev(adapter);
	hw = &adapter->hw;

	devcaps = vzalloc(IXGBE_ACI_MAX_BUF_LEN);
	if (!devcaps)
		return -ENOMEM;

	status = ixgbe_aci_list_caps(hw, devcaps, IXGBE_ACI_MAX_BUF_LEN, NULL,
				     ixgbe_aci_opc_list_dev_caps);
	if (status) {
		dev_dbg(dev,
			"ixgbe_aci_list_caps: failed to read device capabilities, err %d aci_err %d\n",
			status, hw->aci.last_status);
		NL_SET_ERR_MSG_MOD(extack,
				   "Failed to read device capabilities");
		vfree(devcaps);
		return status;
	}

	*data = (u8 *)devcaps;

	return 0;
}
#endif /* HAVE_DEVLINK_REGION_OPS_SNAPSHOT */

static const struct devlink_region_ops ixgbe_nvm_region_ops = {
	.name = "nvm-flash",
	.destructor = vfree,
#ifdef HAVE_DEVLINK_REGION_OPS_SNAPSHOT
	.snapshot = ixgbe_devlink_nvm_snapshot,
#endif
};

static const struct devlink_region_ops ixgbe_sram_region_ops = {
	.name = "shadow-ram",
	.destructor = vfree,
#ifdef HAVE_DEVLINK_REGION_OPS_SNAPSHOT
	.snapshot = ixgbe_devlink_sram_snapshot,
#endif
};

static const struct devlink_region_ops ixgbe_devcaps_region_ops = {
	.name = "device-caps",
	.destructor = vfree,
#ifdef HAVE_DEVLINK_REGION_OPS_SNAPSHOT
	.snapshot = ixgbe_devlink_devcaps_snapshot,
#endif
};

/**
 * ixgbe_devlink_init_regions - Initialize devlink regions
 * @adapter: adapter instance
 *
 * Create devlink regions used to enable access to dump the contents of the
 * flash memory on the device.
 */
void ixgbe_devlink_init_regions(struct ixgbe_adapter *adapter)
{
	struct device *dev = ixgbe_pf_to_dev(adapter);
	struct devlink *devlink = adapter->devlink;
	u64 nvm_size, sram_size;

	nvm_size = adapter->hw.flash.flash_size;
	adapter->nvm_region =
		devlink_region_create(devlink,
				      &ixgbe_nvm_region_ops,
				      1, nvm_size);
	if (IS_ERR(adapter->nvm_region)) {
		dev_err(dev,
			"Failed to create NVM devlink region, err %ld\n",
			PTR_ERR(adapter->nvm_region));
		adapter->nvm_region = NULL;
	}

	sram_size = adapter->hw.flash.sr_words * 2u;
	adapter->sram_region =
		devlink_region_create(devlink,
				      &ixgbe_sram_region_ops,
				      1, sram_size);
	if (IS_ERR(adapter->sram_region)) {
		dev_err(dev,
			"Failed to create shadow-ram devlink region, err %ld\n",
			PTR_ERR(adapter->sram_region));
		adapter->sram_region = NULL;
	}

	adapter->devcaps_region =
		devlink_region_create(devlink,
				      &ixgbe_devcaps_region_ops,
				      10, IXGBE_ACI_MAX_BUF_LEN);
	if (IS_ERR(adapter->devcaps_region)) {
		dev_err(dev,
			"Failed to create device-caps devlink region, err %ld\n",
			PTR_ERR(adapter->devcaps_region));
		adapter->devcaps_region = NULL;
	}
}

/**
 * ixgbe_devlink_destroy_regions - Destroy devlink regions
 * @adapter: adapter instance
 *
 * Remove previously created regions for this PF.
 */
void ixgbe_devlink_destroy_regions(struct ixgbe_adapter *adapter)
{
	if (adapter->nvm_region)
		devlink_region_destroy(adapter->nvm_region);

	if (adapter->sram_region)
		devlink_region_destroy(adapter->sram_region);

	if (adapter->devcaps_region)
		devlink_region_destroy(adapter->devcaps_region);
}
#endif /* HAVE_DEVLINK_REGIONS */

#endif /* CONFIG_NET_DEVLINK */
