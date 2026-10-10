/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (C) 1999 - 2026 Intel Corporation */

#include "ixgbe.h"

#include <linux/uuid.h>
#include <linux/crc32.h>
#if IS_ENABLED(CONFIG_PLDMFW)
#include <linux/pldmfw.h>
#else
#include "kcompat_pldmfw.h"
#endif

#include "ixgbe_fw_update.h"

struct ixgbe_fwu_priv {
	struct pldmfw context;

	struct ixgbe_adapter *adapter;
	struct netlink_ext_ack *extack;

	/* Track which NVM banks to activate at the end of the update */
	u8 activate_flags;

	/* Track the firmware response of the required reset to complete the
	 * flash update.
	 *
	 * 0 - IXGBE_ACI_NVM_POR_FLAG - A full power on is required
	 * 1 - IXGBE_ACI_NVM_PERST_FLAG - A cold PCIe reset is required
	 * 2 - IXGBE_ACI_NVM_EMPR_FLAG - An EMP reset is required
	 */
	u8 reset_level;

	/* Track if EMP reset is available */
	u8 emp_reset_available;
};

/**
 * ixgbe_send_package_data - Send record package data to firmware
 * @context: PLDM fw update structure
 * @data: pointer to the package data
 * @length: length of the package data
 *
 * Send a copy of the package data associated with the PLDM record matching
 * this device to the firmware.
 *
 * Note that this function sends an ACI command that will fail unless the
 * NVM resource has been acquired.
 *
 * Return: 0 on success, or a negative error code on failure.
 */
static int ixgbe_send_package_data(struct pldmfw *context,
				   const u8 *data, u16 length)
{
	struct ixgbe_fwu_priv *priv = container_of(context,
						   struct ixgbe_fwu_priv,
						   context);
	struct netlink_ext_ack *extack = priv->extack;
	struct ixgbe_adapter *adapter = priv->adapter;
	struct ixgbe_hw *hw = &adapter->hw;
	struct device *dev = context->dev;
	u8 *package_data;
	s32 status;

	dev_dbg(dev, "Sending PLDM record package data to firmware\n");

	package_data = kmemdup(data, length, GFP_KERNEL);
	if (!package_data)
		return -ENOMEM;

	status = ixgbe_nvm_set_pkg_data(hw, false, package_data, length);

	kfree(package_data);

	if (status) {
		dev_err(dev,
			"Failed to send record package data to firmware, err %d aci_err %d\n",
			status, hw->aci.last_status);
		NL_SET_ERR_MSG_MOD(extack,
				   "Failed to record package data to firmware");
		return -EIO;
	}

	return 0;
}

/**
 * ixgbe_check_component_response - Report firmware response to a component
 * @adapter: device private data structure
 * @id: component id being checked
 * @response: indicates whether this component can be updated
 * @code: code indicating reason for response
 * @extack: netlink extended ACK structure
 *
 * Check whether firmware indicates if this component can be updated. Report
 * a suitable error message over the netlink extended ACK if the component
 * cannot be updated.
 *
 * Return: 0 if the component can be updated, or -ECANCELED of the
 * firmware indicates the component cannot be updated.
 */
static int ixgbe_check_component_response(struct ixgbe_adapter *adapter,
					  u16 id, u8 response, u8 code,
					  struct netlink_ext_ack *extack)
{
	struct device *dev = ixgbe_pf_to_dev(adapter);
	const char *component;

	switch (id) {
	case NVM_COMP_ID_OROM:
		component = "fw.undi";
		break;
	case NVM_COMP_ID_NVM:
		component = "fw.mgmt";
		break;
	case NVM_COMP_ID_NETLIST:
		component = "fw.netlist";
		break;
	default:
		WARN(1, "Unexpected unknown component identifier 0x%02x", id);
		return -EINVAL;
	}

	dev_dbg(dev, "%s: firmware response 0x%x, code 0x%x\n",
		component, response, code);

	switch (response) {
	case IXGBE_ACI_NVM_PASS_COMP_CAN_BE_UPDATED:
		/* firmware indicated this update is good to proceed */
		return 0;
	case IXGBE_ACI_NVM_PASS_COMP_CAN_MAY_BE_UPDATEABLE:
		dev_warn(dev,
			 "firmware recommends not updating %s, as it may result in a downgrade. Continuing anyways\n",
			 component);
		return 0;
	case IXGBE_ACI_NVM_PASS_COMP_CAN_NOT_BE_UPDATED:
		dev_info(dev, "firmware has rejected updating %s\n", component);
		break;
	case IXGBE_ACI_NVM_PASS_COMP_PARTIAL_CHECK:
		if (ixgbe_fw_recovery_mode(&adapter->hw))
			return 0;
		break;
	}

	switch (code) {
	case IXGBE_ACI_NVM_PASS_COMP_STAMP_IDENTICAL_CODE:
		dev_err(dev,
			"Component comparison stamp for %s is identical to the running image\n",
			component);
		NL_SET_ERR_MSG_MOD(extack,
				   "Component comparison stamp is identical to running image");
		break;
	case IXGBE_ACI_NVM_PASS_COMP_STAMP_LOWER:
		dev_err(dev,
			"Component comparison stamp for %s is lower than the running image\n",
			component);
		NL_SET_ERR_MSG_MOD(extack,
				   "Component comparison stamp is lower than running image");
		break;
	case IXGBE_ACI_NVM_PASS_COMP_INVALID_STAMP_CODE:
		dev_err(dev,
			"Component comparison stamp for %s is invalid\n",
			component);
		NL_SET_ERR_MSG_MOD(extack,
				   "Component comparison stamp is invalid");
		break;
	case IXGBE_ACI_NVM_PASS_COMP_CONFLICT_CODE:
		dev_err(dev,
			"%s conflicts with a previous component table\n",
			component);
		NL_SET_ERR_MSG_MOD(extack,
				   "Component table conflict occurred");
		break;
	case IXGBE_ACI_NVM_PASS_COMP_PRE_REQ_NOT_MET_CODE:
		dev_err(dev,
			"Pre-requisites for component %s have not been met\n",
			component);
		NL_SET_ERR_MSG_MOD(extack, "Component pre-requisites not met");
		break;
	case IXGBE_ACI_NVM_PASS_COMP_NOT_SUPPORTED_CODE:
		dev_err(dev, "%s is not a supported component\n",
			component);
		NL_SET_ERR_MSG_MOD(extack, "Component not supported");
		break;
	case IXGBE_ACI_NVM_PASS_COMP_CANNOT_DOWNGRADE_CODE:
		dev_err(dev,
			"Security restrictions prevent %s from being downgraded\n",
			component);
		NL_SET_ERR_MSG_MOD(extack, "Component cannot be downgraded");
		break;
	case IXGBE_ACI_NVM_PASS_COMP_INCOMPLETE_IMAGE_CODE:
		dev_err(dev, "Received an incomplete component image for %s\n",
			component);
		NL_SET_ERR_MSG_MOD(extack, "Incomplete component image");
		break;
	case IXGBE_ACI_NVM_PASS_COMP_VER_STR_IDENTICAL_CODE:
		dev_err(dev,
			"Component version for %s is identical to the running image\n",
			component);
		NL_SET_ERR_MSG_MOD(extack,
				   "Component version is identical to running image");
		break;
	case IXGBE_ACI_NVM_PASS_COMP_VER_STR_LOWER_CODE:
		dev_err(dev,
			"Component version for %s is lower than the running image\n",
			component);
		NL_SET_ERR_MSG_MOD(extack,
				   "Component version is lower than the running image");
		break;
	default:
		dev_err(dev, "Unexpected response code 0x02%x for %s\n",
			code, component);
		NL_SET_ERR_MSG_MOD(extack,
				   "Received unexpected response code from firmware");
		break;
	}

	return -ECANCELED;
}

/**
 * ixgbe_send_component_table - Send PLDM component table to firmware
 * @context: PLDM fw update structure
 * @component: the component to process
 * @transfer_flag: relative transfer order of this component
 *
 * Read relevant data from the component and forward it to the device
 * firmware. Check the response to determine if the firmware indicates that
 * the update can proceed.
 *
 * Send ACI commands related to the NVM, and assume
 * the NVM resource has been acquired.
 *
 * Return: 0 on success, or a negative error code on failure.
 */
static int ixgbe_send_component_table(struct pldmfw *context,
				      struct pldmfw_component *component,
				      u8 transfer_flag)
{
	struct ixgbe_fwu_priv *priv = container_of(context,
						   struct ixgbe_fwu_priv,
						   context);
	struct ixgbe_adapter *adapter = priv->adapter;
	struct netlink_ext_ack *extack = priv->extack;
	struct ixgbe_aci_cmd_nvm_comp_tbl *comp_tbl;
	u8 comp_response, comp_response_code;
	struct ixgbe_hw *hw = &adapter->hw;
	struct device *dev = context->dev;
	size_t length;
	s32 status;

	switch (component->identifier) {
	case NVM_COMP_ID_OROM:
	case NVM_COMP_ID_NVM:
	case NVM_COMP_ID_NETLIST:
		break;
	default:
		dev_err(dev,
			"Unable to update due to a firmware component with unknown ID %u\n",
			component->identifier);
		NL_SET_ERR_MSG_MOD(extack,
				   "Unable to update due to unknown firmware component");
		return -EOPNOTSUPP;
	}

	length = struct_size(comp_tbl, cvs, component->version_len);
	comp_tbl = (struct ixgbe_aci_cmd_nvm_comp_tbl *)
		    ixgbe_malloc(hw, length);
	if (!comp_tbl)
		return -ENOMEM;

	comp_tbl->comp_class = cpu_to_le16(component->classification);
	comp_tbl->comp_id = cpu_to_le16(component->identifier);
	comp_tbl->comp_class_idx = FWU_COMP_CLASS_IDX_NOT_USE;
	comp_tbl->comp_cmp_stamp = cpu_to_le32(component->comparison_stamp);
	comp_tbl->cvs_type = component->version_type;
	comp_tbl->cvs_len = component->version_len;

	memcpy(comp_tbl->cvs, component->version_string,
	       component->version_len);

	dev_dbg(dev, "Sending component table to firmware\n");

	status = ixgbe_nvm_pass_component_tbl(hw, (u8 *)comp_tbl, length,
					      transfer_flag, &comp_response,
					      &comp_response_code);

	ixgbe_free(hw, comp_tbl);

	if (status) {
		dev_err(dev,
			"Failed to transfer component table to firmware, err %d\n",
			status);
		NL_SET_ERR_MSG_MOD(extack,
				   "Failed to transfer component table to firmware");
		return -EIO;
	}

	return ixgbe_check_component_response(adapter,
					      component->identifier,
					      comp_response,
					      comp_response_code, extack);
}

/**
 * ixgbe_write_one_nvm_block - Write an NVM block and await completion response
 * @adapter: the PF data structure
 * @module: the module to write to
 * @offset: offset in bytes
 * @block_size: size of the block to write, up to 4k
 * @block: pointer to block of data to write
 * @last_cmd: whether this is the last command
 * @reset_level: storage for reset level required
 * @extack: netlink extended ACK structure
 *
 * Write a block of data to a flash module, and await for the completion
 * response message from firmware.
 *
 * Note this function assumes the caller has acquired the NVM resource.
 *
 * On successful return, reset level indicates the device reset required to
 * complete the update.
 *
 *   0 - IXGBE_ACI_NVM_POR_FLAG - A full power on is required
 *   1 - IXGBE_ACI_NVM_PERST_FLAG - A cold PCIe reset is required
 *   2 - IXGBE_ACI_NVM_EMPR_FLAG - An EMP reset is required
 *
 * Return: 0 on success, or a negative error code on failure.
 */
int ixgbe_write_one_nvm_block(struct ixgbe_adapter *adapter,
			      u16 module, u32 offset,
			      u16 block_size, u8 *block, bool last_cmd,
			      u8 *reset_level,
			      struct netlink_ext_ack *extack)
{
	struct device *dev = ixgbe_pf_to_dev(adapter);
	struct ixgbe_hw *hw = &adapter->hw;
	s32 status;

	dev_dbg(dev, "Writing block of %u bytes for module 0x%02x at offset %u\n",
		block_size, module, offset);
	status = ixgbe_aci_update_nvm(hw, module, offset, block_size, block,
				      last_cmd, 0);
	if (status) {
		dev_err(dev, "Failed to flash module 0x%02x with block of size %u at offset %u, err %d\n",
			module, block_size, offset, status);
		NL_SET_ERR_MSG_MOD(extack, "Failed to program flash module");
		return -EIO;
	}
	/* For the last command to write the NVM bank, newer versions of
	 * firmware indicate the required level of reset to complete
	 * activation of firmware. If the firmware supports this, cache the
	 * response for indicating to the user later. Otherwise, assume that
	 * a full power cycle is required.
	 */
	if (reset_level && last_cmd && module == E610_SR_1ST_NVM_BANK_PTR) {
		if (hw->dev_caps.common_cap.pcie_reset_avoidance) {
			*reset_level = IXGBE_ACI_NVM_RESET_LVL_M;
			dev_dbg(dev, "Required reset level is %u\n",
				*reset_level);
		} else {
			*reset_level = IXGBE_ACI_NVM_POR_FLAG;
			dev_dbg(dev, "Firmware doesn't support indicating required reset level. Assuming a power cycle is required\n");
		}
	}

	return 0;
}

/**
 * ixgbe_write_nvm_module - Write data to an NVM module
 * @adapter: the PF driver structure
 * @module: the module id to program
 * @component: the name of the component being updated
 * @image: buffer of image data to write to the NVM
 * @length: length of the buffer
 * @reset_level: storage for reset level required
 * @extack: netlink extended ACK structure
 *
 * Loop over the data for a given NVM module and program it in 4 Kb
 * blocks. Notify devlink core of progress after each block is programmed.
 * Loops over a block of data and programs the NVM in 4k block chunks.
 *
 * Note this function assumes the caller has acquired the NVM resource.
 *
 * Return: 0 on success, or a negative error code on failure.
 */
static int ixgbe_write_nvm_module(struct ixgbe_adapter *adapter, u16 module,
				  const char *component, const u8 *image,
				  u32 length, u8 *reset_level,
				  struct netlink_ext_ack *extack)
{
	struct device *dev = ixgbe_pf_to_dev(adapter);
	struct devlink *devlink = adapter->devlink;
	u32 offset = 0;
	bool last_cmd;
	u8 *block;
	int err;

	dev_dbg(dev, "Beginning write of flash component '%s', module 0x%02x\n",
		component, module);

	devlink_flash_update_status_notify(devlink, "Flashing",
					   component, 0, length);

	block = ixgbe_malloc(hw, IXGBE_ACI_MAX_BUFFER_SIZE);
	if (!block)
		return -ENOMEM;

	do {
		u32 block_size;

		block_size = min_t(u32, IXGBE_ACI_MAX_BUFFER_SIZE,
				   length - offset);
		last_cmd = !(offset + block_size < length);

		memcpy(block, image + offset, block_size);

		err = ixgbe_write_one_nvm_block(adapter, module, offset,
						block_size, block, last_cmd,
						reset_level, extack);
		if (err)
			break;

		offset += block_size;

		devlink_flash_update_status_notify(devlink, "Flashing",
						   component, offset, length);
	} while (!last_cmd);

	dev_dbg(dev, "Completed write of flash component '%s' module 0x%02x\n",
		component, module);

	if (err)
		devlink_flash_update_status_notify(devlink, "Flashing failed",
						   component, length, length);
	else
		devlink_flash_update_status_notify(devlink, "Flashing done",
						   component, length, length);

	ixgbe_free(hw, block);

	return err;
}

/* Length in seconds to wait before timing out when erasing a flash module.
 * Yes, erasing really can take minutes to complete.
 */
#define IXGBE_FW_ERASE_TIMEOUT 300

/**
 * ixgbe_erase_nvm_module - Erase an NVM module and await firmware completion
 * @adapter: the PF data structure
 * @module: the module to erase
 * @component: name of the component being updated
 * @extack: netlink extended ACK structure
 *
 * Erase the inactive NVM bank associated with this module, and await for
 * a completion response message from firmware.
 *
 * Note this function assumes the caller has acquired the NVM resource.
 *
 * Return: 0 on success, or a negative error code on failure.
 */
static int ixgbe_erase_nvm_module(struct ixgbe_adapter *adapter, u16 module,
				  const char *component,
				  struct netlink_ext_ack *extack)
{
	struct device *dev = ixgbe_pf_to_dev(adapter);
	struct devlink *devlink = adapter->devlink;
	struct ixgbe_hw *hw = &adapter->hw;
	s32 status;
	int err;

	dev_dbg(dev, "Beginning erase of flash component '%s', module 0x%02x\n",
		component, module);

	devlink_flash_update_timeout_notify(devlink, "Erasing", component,
					    IXGBE_FW_ERASE_TIMEOUT);

	status = ixgbe_aci_erase_nvm(hw, module);
	if (status) {
		dev_err(dev, "Failed to erase %s (module 0x%02x), err %d\n",
			component, module, status);
		NL_SET_ERR_MSG_MOD(extack, "Failed to erase flash module");
		err = -EIO;

		devlink_flash_update_status_notify(devlink, "Erasing failed",
						   component, 0, 0);
	} else {
		err = 0;

		devlink_flash_update_status_notify(devlink, "Erasing done",
						   component, 0, 0);
	}

	return err;
}

/**
 * ixgbe_switch_flash_banks - Tell firmware to switch NVM banks
 * @adapter: Pointer to the PF data structure
 * @activate_flags: flags used for the activation command
 * @emp_reset_available: on return, indicates if EMP reset is available
 * @extack: netlink extended ACK structure
 *
 * Notify firmware to activate the newly written flash banks, and wait for the
 * firmware response.
 *
 * Return: 0 on success or an error code on failure.
 */
static int ixgbe_switch_flash_banks(struct ixgbe_adapter *adapter,
				    u8 activate_flags,
				    u8 *emp_reset_available,
				    struct netlink_ext_ack *extack)
{
	struct device *dev = ixgbe_pf_to_dev(adapter);
	struct ixgbe_hw *hw = &adapter->hw;
	u8 response_flags;
	s32 status;

	status = ixgbe_nvm_write_activate(hw, activate_flags, &response_flags);
	if (status) {
		dev_err(dev, "Failed to switch active flash banks, err %d\n",
			status);
		NL_SET_ERR_MSG_MOD(extack,
				   "Failed to switch active flash banks");
		return -EIO;
	}
	/* Newer versions of firmware have support to indicate whether an EMP
	 * reset to reload firmware is available. For older firmware, EMP
	 * reset is always available.
	 */
	if (emp_reset_available) {
		if (hw->dev_caps.common_cap.reset_restrict_support) {
			*emp_reset_available =
				response_flags & IXGBE_ACI_NVM_EMPR_ENA;
			dev_dbg(dev,
				"Firmware indicated that EMP reset is %s\n",
				*emp_reset_available ?
				"available" : "not available");
		} else {
			*emp_reset_available = IXGBE_ACI_NVM_EMPR_ENA;
			dev_dbg(dev,
				"Firmware does not support restricting EMP reset availability\n");
		}
	}

	return 0;
}

/**
 * ixgbe_flash_component - Flash a component of the NVM
 * @context: PLDM fw update structure
 * @component: the component table to program
 *
 * Program the flash contents for a given component. First, determine the
 * module id. Then, erase the secondary bank for this module. Finally, write
 * the contents of the component to the NVM.
 *
 * Note this function assumes the caller has acquired the NVM resource.
 *
 * Return: 0 on success, or a negative error code on failure.
 */
static int ixgbe_flash_component(struct pldmfw *context,
				 struct pldmfw_component *component)
{
	struct ixgbe_fwu_priv *priv = container_of(context,
						   struct ixgbe_fwu_priv,
						   context);
	struct netlink_ext_ack *extack = priv->extack;
	struct ixgbe_adapter *adapter = priv->adapter;
	const char *name;
	u8 *reset_level;
	u16 module;
	int err;
	u8 flag;

	switch (component->identifier) {
	case NVM_COMP_ID_OROM:
		module = E610_SR_1ST_OROM_BANK_PTR;
		flag = IXGBE_ACI_NVM_ACTIV_SEL_OROM;
		reset_level = NULL;
		name = "fw.undi";
		break;
	case NVM_COMP_ID_NVM:
		module = E610_SR_1ST_NVM_BANK_PTR;
		flag = IXGBE_ACI_NVM_ACTIV_SEL_NVM;
		reset_level = &priv->reset_level;
		name = "fw.mgmt";
		break;
	case NVM_COMP_ID_NETLIST:
		module = E610_SR_NETLIST_BANK_PTR;
		flag = IXGBE_ACI_NVM_ACTIV_SEL_NETLIST;
		reset_level = NULL;
		name = "fw.netlist";
		break;

	default:
		/* This should not trigger, since we check the id before
		 * sending the component table to firmware.
		 */
		WARN(1, "Unexpected unknown component identifier 0x%02x",
		     component->identifier);
		return -EINVAL;
	}

	/* Mark this component for activating at the end */
	priv->activate_flags |= flag;

	err = ixgbe_erase_nvm_module(adapter, module, name, extack);
	if (err)
		return err;

	return ixgbe_write_nvm_module(adapter, module, name,
				      component->component_data,
				      component->component_size, reset_level,
				      extack);
}

/**
 * ixgbe_finalize_update - Perform last steps to complete device update
 * @context: PLDM fw update structure
 *
 * Called as the last step of the update process. Complete the update by
 * telling the firmware to switch active banks, and perform a reset of
 * configured.
 *
 * Return: 0 on success, or an error code on failure.
 */
static int ixgbe_finalize_update(struct pldmfw *context)
{
	struct ixgbe_fwu_priv *priv = container_of(context,
						   struct ixgbe_fwu_priv,
						    context);
	struct ixgbe_adapter *adapter = priv->adapter;
	struct netlink_ext_ack *extack = priv->extack;
	struct devlink *devlink;
	struct device *dev;
	int err;

	/* Finally, notify firmware to activate the written NVM banks */
	err = ixgbe_switch_flash_banks(adapter, priv->activate_flags,
				       &priv->emp_reset_available, extack);
	if (err)
		return err;

	devlink = adapter->devlink;
	dev = ixgbe_pf_to_dev(adapter);

	/* If the required reset is EMPR, but EMPR is disabled, report that
	 * a reboot is required instead.
	 */
	if (priv->reset_level == IXGBE_ACI_NVM_EMPR_FLAG &&
	    !priv->emp_reset_available) {
		dev_dbg(dev,
			"Firmware indicated EMP reset as sufficient, but EMP reset is disabled\n");
		priv->reset_level = IXGBE_ACI_NVM_PERST_FLAG;
	}

	switch (priv->reset_level) {
	case IXGBE_ACI_NVM_EMPR_FLAG:
		devlink_flash_update_status_notify(devlink,
						   "Activate new firmware by devlink reload",
						   NULL, 0, 0);
		break;
	case IXGBE_ACI_NVM_PERST_FLAG:
		devlink_flash_update_status_notify(devlink,
						   "Activate new firmware by rebooting the system",
						   NULL, 0, 0);
		break;
	case IXGBE_ACI_NVM_POR_FLAG:
		devlink_flash_update_status_notify(devlink,
						   "Activate new firmware by power cycling the system",
						   NULL, 0, 0);
		break;
	default:
		devlink_flash_update_status_notify(devlink,
						   "Suggested is to activate new firmware by devlink reload, if it doesn't work then a power cycle is required",
						   NULL, 0, 0);
		break;
	}

	adapter->fw_emp_reset_disabled = !priv->emp_reset_available;

	return 0;
}

static const struct pldmfw_ops ixgbe_fwu_ops_e610 = {
	.match_record = &pldmfw_op_pci_match_record,
	.send_package_data = &ixgbe_send_package_data,
	.send_component_table = &ixgbe_send_component_table,
	.flash_component = &ixgbe_flash_component,
	.finalize_update = &ixgbe_finalize_update,
};

/**
 * ixgbe_get_pending_updates - Check if the component has a pending update
 * @adapter: the PF driver structure
 * @pending: on return, bitmap of updates pending
 * @extack: Netlink extended ACK
 *
 * Check if the device has any pending updates on any flash components.
 *
 * Return: 0 on success, or a negative error code on failure.
 * Updates pending with the bitmap of pending updates.
 */
int ixgbe_get_pending_updates(struct ixgbe_adapter *adapter, u8 *pending,
			      struct netlink_ext_ack *extack)
{
	struct device *dev = ixgbe_pf_to_dev(adapter);
	struct ixgbe_hw_dev_caps *dev_caps;
	struct ixgbe_hw *hw = &adapter->hw;
	s32 status;

	dev_caps = ixgbe_malloc(hw, sizeof(*dev_caps));
	if (!dev_caps)
		return -ENOMEM;

	status = ixgbe_discover_dev_caps(hw, dev_caps);
	if (status) {
		NL_SET_ERR_MSG_MOD(extack,
				   "Unable to read device capabilities");
		ixgbe_free(hw, dev_caps);
		return -EIO;
	}

	*pending = 0;

	if (dev_caps->common_cap.nvm_update_pending_nvm) {
		dev_info(dev,
			 "The fw.mgmt flash component has a pending update\n");
		*pending |= IXGBE_ACI_NVM_ACTIV_SEL_NVM;
	}

	if (dev_caps->common_cap.nvm_update_pending_orom) {
		dev_info(dev,
			 "The fw.undi flash component has a pending update\n");
		*pending |= IXGBE_ACI_NVM_ACTIV_SEL_OROM;
	}

	if (dev_caps->common_cap.nvm_update_pending_netlist) {
		dev_info(dev,
			 "The fw.netlist flash component has a pending update\n");
		*pending |= IXGBE_ACI_NVM_ACTIV_SEL_NETLIST;
	}

	ixgbe_free(hw, dev_caps);

	return 0;
}

/**
 * ixgbe_cancel_pending_update - Cancel any pending update for a component
 * @adapter: the PF driver structure
 * @component: if not NULL, the name of the component being updated
 * @extack: Netlink extended ACK structure
 *
 * Cancel any pending update for the specified component. If component is
 * NULL, all device updates will be canceled.
 *
 * Return: 0 on success, or a negative error code on failure.
 */
static int ixgbe_cancel_pending_update(struct ixgbe_adapter *adapter,
				       const char *component,
				       struct netlink_ext_ack *extack)
{
	struct devlink *devlink = adapter->devlink;
	struct ixgbe_hw *hw = &adapter->hw;
	s32 status;
	u8 pending;
	int err;

	err = ixgbe_get_pending_updates(adapter, &pending, extack);
	if (err)
		return err;

	/* If the flash_update request is for a specific component, ignore all
	 * of the other components.
	 */
	if (component) {
		if (strcmp(component, "fw.mgmt") == 0)
			pending &= IXGBE_ACI_NVM_ACTIV_SEL_NVM;
		else if (strcmp(component, "fw.undi") == 0)
			pending &= IXGBE_ACI_NVM_ACTIV_SEL_OROM;
		else if (strcmp(component, "fw.netlist") == 0)
			pending &= IXGBE_ACI_NVM_ACTIV_SEL_NETLIST;
		else
			WARN(1, "Unexpected flash component %s", component);
	}

	/* There is no previous pending update, so this request may continue */
	if (!pending)
		return 0;

	/* In order to allow overwriting a previous pending update, notify
	 * firmware to cancel that update by issuing the appropriate command.
	 */
	devlink_flash_update_status_notify(devlink,
					   "Canceling previous pending update",
					   component, 0, 0);

	status = ixgbe_acquire_nvm(hw, IXGBE_RES_WRITE);
	if (status) {
		dev_err(ixgbe_pf_to_dev(adapter),
			"Failed to acquire device flash lock, err %d\n",
			status);
		NL_SET_ERR_MSG_MOD(extack,
				   "Failed to acquire device flash lock");
		return -EIO;
	}

	pending |= IXGBE_ACI_NVM_REVERT_LAST_ACTIV;
	err = ixgbe_switch_flash_banks(adapter, pending, NULL, extack);

	ixgbe_release_nvm(hw);

	/* Since we've canceled the pending update, we no longer know if EMP
	 * reset is restricted.
	 */
	adapter->fw_emp_reset_disabled = false;

	return err;
}

/**
 * ixgbe_flash_pldm_image - Write a PLDM-formatted firmware image to the device
 * @devlink: pointer to devlink associated with the device to update
 * @params: devlink flash update parameters
 * @extack: netlink extended ACK structure
 *
 * Parse the data for a given firmware file, verifying that it is a valid PLDM
 * formatted image that matches this device.
 *
 * Extract the device record Package Data and Component Tables and send them
 * to the firmware. Extract and write the flash data for each of the three
 * main flash components, "fw.mgmt", "fw.undi", and "fw.netlist". Notify
 * firmware once the data is written to the inactive banks.
 *
 * Return: 0 on success or a negative error code on failure.
 */
int ixgbe_flash_pldm_image(struct devlink *devlink,
			   struct devlink_flash_update_params *params,
			   struct netlink_ext_ack *extack)
{
	struct ixgbe_adapter *adapter;
	struct ixgbe_fwu_priv priv;
#ifndef HAVE_DEVLINK_FLASH_UPDATE_PARAMS_FW
	const struct firmware *fw;
#endif
	struct ixgbe_hw *hw;
	struct device *dev;
	u8 preservation;
	s32 status;
	int err;

	adapter = *(struct ixgbe_adapter **)devlink_priv(devlink);
	dev = ixgbe_pf_to_dev(adapter);
	hw = &adapter->hw;

	switch (params->overwrite_mask) {
	case 0:
		/* preserve all settings and identifiers */
		preservation = IXGBE_ACI_NVM_PRESERVE_ALL;
		break;
	case DEVLINK_FLASH_OVERWRITE_SETTINGS:
		/* overwrite settings, but preserve vital information such as
		 * device identifiers.
		 */
		preservation = IXGBE_ACI_NVM_PRESERVE_SELECTED;
		break;
	case (DEVLINK_FLASH_OVERWRITE_SETTINGS |
	      DEVLINK_FLASH_OVERWRITE_IDENTIFIERS):
		/* overwrite both settings and identifiers, preserve nothing */
		preservation = IXGBE_ACI_NVM_NO_PRESERVATION;
		break;
	default:
		NL_SET_ERR_MSG_MOD(extack,
				   "Requested overwrite mask is not supported");
		return -EOPNOTSUPP;
	}

	if (!hw->dev_caps.common_cap.nvm_unified_update &&
	    !ixgbe_fw_recovery_mode(hw)) {
		NL_SET_ERR_MSG_MOD(extack,
				   "Current firmware does not support unified update");
		return -EOPNOTSUPP;
	}

	memset(&priv, 0, sizeof(priv));

	priv.context.ops = &ixgbe_fwu_ops_e610;
	priv.context.dev = dev;
	priv.extack = extack;
	priv.adapter = adapter;
	priv.activate_flags = preservation;

	devlink_flash_update_status_notify(devlink,
					   "Preparing to flash", NULL, 0, 0);

	err = ixgbe_cancel_pending_update(adapter, NULL, extack);
	if (err)
		return err;

	status = ixgbe_acquire_nvm(hw, IXGBE_RES_WRITE);
	if (status) {
		dev_err(dev,
			"Failed to acquire device flash lock, err %d aci_err %d\n",
			status, hw->aci.last_status);
		NL_SET_ERR_MSG_MOD(extack,
				   "Failed to acquire device flash lock");
		return -EIO;
	}

#ifdef HAVE_DEVLINK_FLASH_UPDATE_PARAMS_FW
	err = pldmfw_flash_image(&priv.context, params->fw);
#else
	err = request_firmware(&fw, params->file_name, dev);
	if (err) {
		NL_SET_ERR_MSG_MOD(extack, "Unable to read file from disk");
		ixgbe_release_nvm(hw);
		return err;
	}

	err = pldmfw_flash_image(&priv.context, fw);

	release_firmware(fw);
#endif
	if (err == -ENOENT) {
		dev_err(dev,
			"Firmware image has no record matching this device\n");
		NL_SET_ERR_MSG_MOD(extack,
				   "Firmware image has no record matching this device");
	} else if (err) {
		/* Do not set a generic extended ACK message here. A more
		 * specific message may already have been set by one of our
		 * ops.
		 */
		dev_err(dev, "Failed to flash PLDM image, err %d", err);
	}

	ixgbe_release_nvm(hw);

	return err;
}
