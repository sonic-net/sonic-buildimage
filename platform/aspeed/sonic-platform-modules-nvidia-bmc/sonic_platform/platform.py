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

"""
SONiC Platform API - Platform class for NVIDIA AST2700 BMC
"""

try:
    from sonic_platform.chassis import Chassis
except ImportError as e:
    raise ImportError(str(e) + " - required module not found")


class Platform:
    """
    Platform class for NVIDIA AST2700 BMC

    Provides access to chassis-level functionality.
    """

    def __init__(self):
        """
        Initialize the Platform object
        """
        self._chassis = Chassis()

    def get_chassis(self):
        """
        Retrieves the chassis object

        Returns:
            An object derived from ChassisBase representing the chassis
        """
        return self._chassis
