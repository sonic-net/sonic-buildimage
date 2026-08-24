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

import os
from pathlib import Path
import pytest


platform = os.environ.get("CONFIGURED_PLATFORM", None)
platform = platform.lower() if platform else platform
if platform is None:
    raise Exception("CONFIGURED_PLATFORM environment variable is not set")
if not platform in ("mellanox", "nvidia-bluefield"):
    raise Exception(
        f"Invalid environment variable value for CONFIGURED_PLATFORM: \"{platform}\"."
        " Expected \"mellanox\" or \"nvidia-bluefield\"."
    )
is_bluefield = platform == "nvidia-bluefield"


def pytest_collection_modifyitems(config, items):
    if is_bluefield:
        # Skip the bfb installer tests if the platform is nvidia-bluefield.
        # That platform is the DPU platform. The dpu-installer is designed to work from the main
        # host of a Smart Switch.
        conftest_dir = Path(__file__).parent.resolve()
        skip_marker = pytest.mark.skip(
            reason="Skipping because dpu-installer not supported on nvidia-bluefield platform"
        )
        for item in items:
            if conftest_dir in item.path.parents:
                # Paranoid checks: Don't accidentally skip other tests! Ensure the test to skip
                # is in THIS directory. Ensure the platform is nvidia-bluefield.
                assert item.path.parts[-2] == conftest_dir.name == "mellanox_bfb_installer"
                assert platform == "nvidia-bluefield"
                item.add_marker(skip_marker)
