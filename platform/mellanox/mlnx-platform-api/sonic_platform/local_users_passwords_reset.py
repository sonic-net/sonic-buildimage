#
# Copyright (c) 2019-2024 NVIDIA CORPORATION & AFFILIATES.
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
# Module contains an implementation of SONiC Platform Base API and
# provides the local users' passwords reset functionality implementation.
#
#############################################################################

try:
    from sonic_platform_base.local_users_passwords_reset_base import LocalUsersConfigurationResetBase
    from sonic_py_common.logger import Logger
    from . import utils
except ImportError as e:
    raise ImportError (str(e) + "- required module not found")


# Global logger class instance
logger = Logger()


LONG_REBOOT_PRESS_FILEPATH = '/var/run/hw-management/system/reset_long_pb'


class LocalUsersConfigurationReset(LocalUsersConfigurationResetBase):
    def should_trigger(self):
        '''
        The condition for triggering passwords reset is by checking if the
        reboot cause was a long reboot press. 
        '''
        try:
            status = utils.read_int_from_file(LONG_REBOOT_PRESS_FILEPATH, raise_exception=True)
            return True if status ==  1 else False
        except (ValueError, IOError) as e:
            logger.log_error(f"Failed to read long reboot press from {LONG_REBOOT_PRESS_FILEPATH} - {e}")
            return False
