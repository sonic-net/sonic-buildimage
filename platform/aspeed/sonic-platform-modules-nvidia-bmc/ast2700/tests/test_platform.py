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

from unittest.mock import MagicMock, patch


class TestPlatform:
    """Minimal checks that `Platform` instantiates and owns a chassis."""

    def test_platform_constructs_chassis(self):
        sentinel = MagicMock(name="Chassis-sentinel")
        with patch("sonic_platform.platform.Chassis", return_value=sentinel) as chassis_cls:
            from sonic_platform.platform import Platform
            platform = Platform()

        chassis_cls.assert_called_once_with()
        assert platform.get_chassis() is sentinel
