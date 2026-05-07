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
Setup script for Mellanox Firmware Manager package.
"""

import os
from setuptools import setup
import sys


platform = os.environ.get("CONFIGURED_PLATFORM", None)
platform = platform.lower() if platform else platform
if platform is None:
    sys.stderr.write("CONFIGURED_PLATFORM environment variable is not set")
    sys.exit(1)
if not platform in ("mellanox", "nvidia-bluefield"):
    sys.stderr.write(f"Invalid CONFIGURED_PLATFORM: \"{platform}\". Expected \"mellanox\" or \"nvidia-bluefield\".")
    sys.exit(1)
is_bluefield = platform == "nvidia-bluefield"


setup(
    name="mellanox-platform-utils",
    version="1.0.0",
    author="Oleksandr Ivantsiv",
    author_email="oivantsiv@nvidia.com",
    description="Platform utilities package for Mellanox ASICs",
    url="https://github.com/sonic-net/sonic-buildimage",
    packages=[
        *([] if is_bluefield else ["mellanox_bfb_installer"]),
        "mellanox_component_versions",
        "mellanox_fw_manager",
    ],
    classifiers=[
        "Development Status :: 4 - Beta",
        "Intended Audience :: Developers",
        "Operating System :: POSIX :: Linux",
        "Programming Language :: Python :: 3",
        "Programming Language :: Python :: 3.7",
        "Programming Language :: Python :: 3.8",
        "Programming Language :: Python :: 3.9",
        "Topic :: System :: Hardware",
        "Topic :: System :: Systems Administration",
    ],
    python_requires=">=3.10",
    install_requires=[
        "tabulate",
        "click>=7.0",
    ],
    extras_require={
        "testing": [
            "pytest>=6.0",
            "pytest-cov>=2.10.0",
            "pytest-mock>=3.3.0",
        ],
    },
    test_suite="tests",
    entry_points={
        "console_scripts": [
            "mlnx-fw-manager=mellanox_fw_manager.main:main",
            "get_component_versions.py=mellanox_component_versions.main:main",
            *([] if is_bluefield else ["sonic-bfb-installer=mellanox_bfb_installer.main:main"]),
        ],
    },
    include_package_data=True,
    zip_safe=False,
)
