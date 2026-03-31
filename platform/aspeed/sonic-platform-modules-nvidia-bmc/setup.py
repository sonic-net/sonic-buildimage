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
from setuptools import setup

setup(
    name="sonic-platform",
    version="1.0",
    description="SONiC platform API implementation on NVIDIA Aspeed BMC Platforms",
    license="Apache 2.0",
    author="NVIDIA",
    author_email="willtsai@nvidia.com",
    url="https://github.com/sonic-net/sonic-buildimage",
    packages=["sonic_platform"],
    package_dir={"sonic_platform": "sonic_platform"},
    classifiers=[
        "Development Status :: 3 - Alpha",
        "Environment :: Plugins",
        "Intended Audience :: Developers",
        "Intended Audience :: Information Technology",
        "Intended Audience :: System Administrators",
        "License :: OSI Approved :: Apache Software License",
        "Natural Language :: English",
        "Operating System :: POSIX :: Linux",
        "Programming Language :: Python :: 3.11",
        "Topic :: Utilities",
    ],
    keywords="sonic SONiC platform PLATFORM aspeed bmc nvidia ast2700",
)
