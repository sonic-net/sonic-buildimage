#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2026 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
# SPDX-License-Identifier: Apache-2.0
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

from unittest import mock

from sonic_platform_base.chassis_base import ChassisBase
from sonic_platform.reboot_cause import RebootCause


class TestRebootCause:

    def test_returns_non_hardware_when_no_cause_set(self):
        with mock.patch('sonic_platform.reboot_cause.hwmgmt_flag_is_set', return_value=False):
            cause, description = RebootCause().get_reboot_cause()
        assert cause == ChassisBase.REBOOT_CAUSE_NON_HARDWARE
        assert description == ""

    def test_reboot_causes(self):
        reboot_cause = RebootCause()
        active_flags = set()

        def flag_side_effect(bmc_dir, filename):
            assert bmc_dir == reboot_cause._BMC_RESET_DIR
            return filename in active_flags

        with mock.patch('sonic_platform.reboot_cause.hwmgmt_flag_is_set', side_effect=flag_side_effect):
            for key, value in reboot_cause._reboot_cause_dict.items():
                active_flags.add(key)
                cause, description = reboot_cause.get_reboot_cause()
                assert cause == value
                assert description == ""
                active_flags.discard(key)
