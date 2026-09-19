#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2025-2026 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
# Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
#############################################################################
# Mellanox
#
# Module contains an implementation of new platform api
#
#############################################################################


try:
    import sys
    import importlib.util
    import os
    import io
    import contextlib
    from sonic_platform_base.bmc_base import BMCBase
    from sonic_platform_base.redfish_client import RedfishClient
    from sonic_py_common.logger import Logger
except ImportError as e:
    raise ImportError (str(e) + "- required module not found")


logger = Logger('bmc')


HW_MGMT_REDFISH_CLIENT_PATH = '/usr/bin/hw_management_redfish_client.py'
HW_MGMT_REDFISH_CLIENT_NAME = 'hw_management_redfish_client'


def _get_hw_mgmt_redfish_client():
    """ Get hw_management_redfish_client module. """
    if HW_MGMT_REDFISH_CLIENT_NAME in sys.modules:
        return sys.modules[HW_MGMT_REDFISH_CLIENT_NAME]
    if not os.path.exists(HW_MGMT_REDFISH_CLIENT_PATH):
        raise ImportError(f"{HW_MGMT_REDFISH_CLIENT_NAME} not found at {HW_MGMT_REDFISH_CLIENT_PATH}")
    spec = importlib.util.spec_from_file_location(HW_MGMT_REDFISH_CLIENT_NAME, HW_MGMT_REDFISH_CLIENT_PATH)
    hw_mgmt_redfish_client = importlib.util.module_from_spec(spec)
    sys.modules[HW_MGMT_REDFISH_CLIENT_NAME] = hw_mgmt_redfish_client
    spec.loader.exec_module(hw_mgmt_redfish_client)
    return hw_mgmt_redfish_client


def _get_bmc_values():
    none_values = None, None, None
    from .device_data import DeviceDataManager
    if not DeviceDataManager.is_platform_with_bmc():
        return none_values
    from sonic_py_common import device_info
    bmc_data = device_info.get_bmc_data()
    bmc_addr = bmc_data.get('bmc_addr')
    if not bmc_addr:
        logger.log_error("BMC address not found in bmc_data")
        return none_values
    bmc_config = device_info.get_bmc_build_config()
    if not bmc_config:
        logger.log_error("BMC build configuration not found")
        return none_values
    bmc_nos_account_username = bmc_config.get('bmc_nos_account_username')
    if not bmc_nos_account_username:
        logger.log_error("BMC NOS account username not found in build configuration")
        return none_values
    bmc_root_account_default_password = bmc_config.get('bmc_root_account_default_password')
    return bmc_addr, bmc_nos_account_username, bmc_root_account_default_password


class BMC(BMCBase):

    """
    BMC encapsulates BMC device functionality.
    It also acts as wrapper of RedfishClient.
    """

    # BMC firmware IDs in the Redfish firmware inventory, in resolution order.
    BMC_FIRMWARE_IDS = ('FW_BMC_0', 'MGX_FW_BMC_0')
    BMC_EEPROM_ID = 'BMC_eeprom'
    _instance = None

    def __init__(self, addr, bmc_nos_account_username, bmc_root_account_default_password):
        super().__init__(addr)
        self._bmc_nos_account_username = bmc_nos_account_username
        self._bmc_root_account_default_password = bmc_root_account_default_password
        self._nos_account_provisioning_tried = False
        self._firmware_id = None

    @staticmethod
    def get_instance():
        if BMC._instance is None:
            bmc_addr, bmc_nos_account_username, bmc_root_account_default_password = _get_bmc_values()
            if not bmc_addr or not bmc_nos_account_username:
                return None
            BMC._instance = BMC(bmc_addr, bmc_nos_account_username, bmc_root_account_default_password)
        return BMC._instance

    def _get_login_user_callback(self):
        return self._bmc_nos_account_username

    def _get_login_password_callback(self):
        return self._get_tpm_password()

    def _get_default_root_password(self):
        return self._bmc_root_account_default_password

    def _login(self):
        if self._nos_account_provisioning_tried:
            return super()._login()
        # ERR_CODE_AUTH_FAILURE on the first login is expected when the BMC NOS account has not been
        # provisioned yet, so report it as an error only if the recovery below fails.
        ret = self.rf_client.login(log_errors=False)
        if ret == RedfishClient.ERR_CODE_OK:
            return ret
        if ret != RedfishClient.ERR_CODE_AUTH_FAILURE:
            logger.log_error(f"Failed to login to the BMC: {ret}")
            return ret
        # If the NOS account has not been provisioned yet, call hw_management_redfish_client.py to do it.
        self._nos_account_provisioning_tried = True
        if not self._configure_nos_account():
            return ret
        return super()._login()

    def _configure_nos_account(self):
        logger.log_notice("Configuring the BMC NOS account from hw_management_redfish_client.py")
        # hw_management_redfish_client reports the login flow on stdout/stderr,
        # keep it out of the CLI output and re-emit it to syslog
        output = io.StringIO()
        try:
            with contextlib.redirect_stdout(output), contextlib.redirect_stderr(output):
                provisioning_ret = _get_hw_mgmt_redfish_client().BMCAccessor().login()
        except Exception as e:
            logger.log_error(f"Error configuring the BMC NOS account from hw_management_redfish_client.py: {str(e)}")
            return False
        finally:
            for line in output.getvalue().splitlines():
                logger.log_notice(f"hw_management_redfish_client: {line}")
        if provisioning_ret != 0:
            logger.log_error(f"Failed to configure the BMC NOS account from hw_management_redfish_client.py: {provisioning_ret}")
            return False
        logger.log_notice("BMC NOS account configured successfully")
        return True

    def _get_firmware_inventory_entry(self):
        """
        Resolve the BMC's own entry in the Redfish firmware inventory.

        Queries each known ID's own inventory-member endpoint directly, in
        resolution order, and stops at the first that exists. This avoids
        listing the full firmware inventory, which would query every
        component's version just to find the BMC's one entry.

        Handles the session instead of @with_session_management: this is also
        called from inside an open session (BMCBase.update_firmware() resolves
        the firmware ID while its own session is held), and the decorator would
        log that session out.

        Returns:
            A tuple (fw_id, version), or (None, None) if none of the known IDs
            could be read from the BMC firmware inventory
        """
        caller_has_session = self.rf_client.has_login()
        try:
            if not caller_has_session:
                ret = self._login()
                if ret != RedfishClient.ERR_CODE_OK:
                    logger.log_error(f"Failed to get the BMC firmware inventory: {ret}")
                    return None, None
            for fw_id in BMC.BMC_FIRMWARE_IDS:
                ret, version = self.rf_client.redfish_api_get_firmware_version(fw_id)
                if ret == RedfishClient.ERR_CODE_OK:
                    return fw_id, version
            logger.log_error(f"None of the known BMC firmware IDs {list(BMC.BMC_FIRMWARE_IDS)} "
                             f"could be read from the BMC firmware inventory")
            return None, None
        except Exception as e:
            logger.log_error(f"Exception in _get_firmware_inventory_entry: {str(e)}")
            return None, None
        finally:
            if not caller_has_session:
                self._logout()

    def get_firmware_id(self):
        """
        Get the BMC firmware ID, resolving and caching it on first use.

        Returns:
            A string containing the BMC firmware ID. Falls back to
            BMC_FIRMWARE_IDS[0] (unresolved and not cached) if the inventory
            could not be read or holds none of the known IDs.
        """
        if self._firmware_id is None:
            self._firmware_id, _ = self._get_firmware_inventory_entry()
        return self._firmware_id or BMC.BMC_FIRMWARE_IDS[0]

    def get_version(self):
        """
        Retrieves the BMC firmware version

        Overrides BMCBase.get_version(): the inventory lookup that finds the BMC's
        own entry already carries the version, so there is nothing left to query.

        Returns:
            A string containing the BMC firmware version.
            Returns 'N/A' if the BMC firmware version cannot be retrieved
        """
        fw_id, version = self._get_firmware_inventory_entry()
        return version if fw_id else 'N/A'

    def _get_eeprom_id(self):
        return BMC.BMC_EEPROM_ID

    def _get_tpm_password(self):
        try:
            return _get_hw_mgmt_redfish_client().BMCAccessor().get_login_password()
        except Exception as e:
            logger.log_error(f"Error getting TPM password from hw_management_redfish_client.py: {str(e)}")
            raise

    def _get_component_list(self):
        from .component import ComponentBMC
        return [ComponentBMC()]
