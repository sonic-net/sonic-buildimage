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
    from functools import wraps
    import sys
    import importlib.util
    import os
    import filelock
    from sonic_platform_base.bmc_base import BMCBase
    from sonic_platform_base.redfish_client import RedfishClient
    from sonic_py_common.logger import Logger
except ImportError as e:
    raise ImportError (str(e) + "- required module not found")


logger = Logger('bmc')


HW_MGMT_REDFISH_CLIENT_PATH = '/usr/bin/hw_management_redfish_client.py'
HW_MGMT_REDFISH_CLIENT_NAME = 'hw_management_redfish_client'


def under_lock(lockfile, timeout=2):
    """ Execute operations under lock. """
    def _under_lock(func):
        @wraps(func)
        def wrapped_function(*args, **kwargs):
            with filelock.FileLock(lockfile, timeout):
                return func(*args, **kwargs)

        return wrapped_function
    return _under_lock


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


class BMC(BMCBase):

    """
    BMC encapsulates BMC device functionality.
    It also acts as wrapper of RedfishClient.
    """

    BMC_FIRMWARE_ID = 'MGX_FW_BMC_0'
    BMC_EEPROM_ID = 'BMC_eeprom'
    BMC_NOS_ACCOUNT = 'yormnAnb'
    BMC_NOS_ACCOUNT_DEFAULT_PASSWORD = "ABYX12#14artb51"
    ROOT_ACCOUNT_DEFAULT_PASSWORD = '0penBmcTempPass!'

    _instance = None

    def __init__(self, addr):
        # Call BMCBase ctor which sets self.addr and self.rf_client
        super().__init__(addr)
        self.__using_tpm_password = True

    @staticmethod
    def get_instance():
        if BMC._instance is None:
            from sonic_py_common import device_info
            bmc_data = device_info.get_bmc_data()
            if not bmc_data:
                # BMC is not present on this platform - missing bmc.json
                return None
            bmc_addr = bmc_data.get('bmc_addr')
            if not bmc_addr:
                logger.log_error("BMC address not found in bmc_data")
                return None
            BMC._instance = BMC(bmc_addr)
        return BMC._instance

    def _get_login_user_callback(self):
        return BMC.BMC_NOS_ACCOUNT

    def _get_login_password_callback(self):
        if self.__using_tpm_password:
            return self._get_tpm_password()
        else:
            return BMC.BMC_NOS_ACCOUNT_DEFAULT_PASSWORD

    def _get_default_root_password(self):
        return BMC.ROOT_ACCOUNT_DEFAULT_PASSWORD

    def _get_firmware_id(self):
        return BMC.BMC_FIRMWARE_ID

    def _get_eeprom_id(self):
        return BMC.BMC_EEPROM_ID

    def _get_tpm_password(self):
        try:
            return _get_hw_mgmt_redfish_client().BMCAccessor().get_login_password()
        except Exception as e:
            logger.log_error(f"Error getting TPM password from hw_management_redfish_client.py: {str(e)}")
            raise

    @under_lock(lockfile='/var/run/bmc_restore_tpm_credential.lock', timeout=5)
    def _restore_tpm_credential(self):
        logger.log_notice(f'Start BMC TPM password recovery flow')
        # We are not good with TPM password here, Try to login with default password
        logger.log_notice(f'Try to login with BMC default password')
        # Indicate password callback function to switch to default password temporarily
        self.__using_tpm_password = False
        ret = self.rf_client.login()
        if ret != RedfishClient.ERR_CODE_OK:
            logger.log_error(f'Bad credential: Fail to login BMC with both TPM based and default passwords')
            # Resume to TPM password
            self.__using_tpm_password = True
            return False

        # Indicate RedfishClient to switch to TPM password
        self.__using_tpm_password = True
        logger.log_notice(f'Login successfully with BMC default password')
        try:
            password = self._get_tpm_password()
        except Exception as e:
            self.rf_client.invalidate_session()
            logger.log_error(f'Fail to get TPM password: {str(e)}')
            return False

        logger.log_notice(f'Try to apply TPM based password to BMC NOS account')
        ret, msg = self._change_login_password(password)
        if ret != RedfishClient.ERR_CODE_OK:
            self.rf_client.invalidate_session()
            logger.log_error(f'Fail to apply TPM based password to BMC NOS account: {msg}')
            return False
        else:
            logger.log_notice(f'TPM password is successfully applied to BMC NOS account')

        return True

    def _get_component_list(self):
        from .component import ComponentBMC
        return [ComponentBMC()]

    def _login(self):
        """
        Override BMCBase _login to implement TPM password recovery flow.
        """
        logger.log_notice(f'Try login to BMC using the NOS account')
        if self.rf_client.has_login():
            return RedfishClient.ERR_CODE_OK
        ret = self.rf_client.login()
        if ret == RedfishClient.ERR_CODE_AUTH_FAILURE:
            logger.log_notice(f'Fail to login BMC with TPM password. Trigger password recovery flow')
            restored = self._restore_tpm_credential()
            if restored:
                ret = RedfishClient.ERR_CODE_OK
        elif ret == RedfishClient.ERR_CODE_PASSWORD_UNAVAILABLE:
            logger.log_notice(f'Fail to get TPM password')
        return ret

    def _change_login_password(self, password, user=None):
        """
        Override BMCBase _change_login_password because we do not want use @with_session_management
        which calls _login and _restore_tpm_credential, in order to prevent infinite loop.
        """
        return self.rf_client.redfish_api_change_login_password(password, user)

    def reset_root_password(self):
        """
        Override BMCBase reset_root_password because we need to call _login and _logout explicitly
        since we override _change_login_password without @with_session_management.
        """
        try:
            self._login()
            # Call BMCBase reset_root_password which calls _change_login_password
            (ret, msg) = super().reset_root_password()
            self._logout()
            return (ret, msg)
        except Exception as e:
            logger.log_error(f'Failed to reset BMC root password: {str(e)}')
            self._logout()
            logger.log_notice(f'Logged out from BMC in exception handler of reset_root_password')
            return (RedfishClient.ERR_CODE_AUTH_FAILURE, str(e))
