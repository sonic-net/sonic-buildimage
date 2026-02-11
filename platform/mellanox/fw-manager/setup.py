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

from setuptools import setup, find_packages
import sys

setup(
    name="mellanox-fw-manager",
    version="1.0.0",
    author="Oleksandr Ivantsiv",
    author_email="oivantsiv@nvidia.com",
    description="Firmware management package for Mellanox ASICs",
    url="https://github.com/Azure/sonic-buildimage",
    packages=["mellanox_fw_manager"],
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
    python_requires=">=3.7",
    install_requires=[
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
        ],
    },
    include_package_data=True,
    zip_safe=False,
)
