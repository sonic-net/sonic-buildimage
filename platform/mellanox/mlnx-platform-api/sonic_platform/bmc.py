#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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
    from sonic_platform_base.bmc_base import BMCBase
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
    from sonic_py_common import device_info
    bmc_data = device_info.get_bmc_data()
    if not bmc_data:
        # BMC is not present on this platform - missing bmc.json
        return none_values
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

    BMC_FIRMWARE_ID = 'MGX_FW_BMC_0'
    BMC_EEPROM_ID = 'BMC_eeprom'
    _instance = None

    def __init__(self, addr, bmc_nos_account_username, bmc_root_account_default_password):
        super().__init__(addr)
        self._bmc_nos_account_username = bmc_nos_account_username
        self._bmc_root_account_default_password = bmc_root_account_default_password

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

    def get_firmware_id(self):
        return BMC.BMC_FIRMWARE_ID

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
