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


platform = os.environ.get("CONFIGURED_PLATFORM", None)
platform = platform.lower() if platform else platform
if platform is None:
    raise Exception("CONFIGURED_PLATFORM environment variable is not set")
SUPPORTED_PLATFORMS = ("mellanox", "nvidia-bluefield", "aspeed")
if platform not in SUPPORTED_PLATFORMS:
    expected = ", ".join(f"\"{p}\"" for p in SUPPORTED_PLATFORMS)
    raise Exception(
        f"Invalid environment variable value for CONFIGURED_PLATFORM: \"{platform}\"."
        f" Expected one of {expected}."
    )

# Keep in step with the packages selection in setup.py.
SKIP_REASONS = {
    "nvidia-bluefield": "dpu-installer not supported on nvidia-bluefield platform",
    "aspeed": "mellanox_bfb_installer not packaged for the SONiC BMC (aspeed)",
}

# Skip collection before test imports on platforms that do not ship the installer.
collect_ignore_glob = ["test_*.py"] if platform in SKIP_REASONS else []
