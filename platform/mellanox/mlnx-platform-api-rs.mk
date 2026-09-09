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

# The Rust platform API is compiled from source into whichever daemon uses it,
# so it produces no package of its own.  This target exists so it can still be
# built and unit-tested on its own:
#
#     make target/files/$(BLDENV)/mlnx-platform-api-rs
#
# which is the counterpart of building the Python platform API's wheel.

MLNX_PLATFORM_API_RS = mlnx-platform-api-rs
$(MLNX_PLATFORM_API_RS)_SRC_PATH = $(PLATFORM_PATH)/mlnx-platform-api-rs
# swss-common's bindgen step needs the headers, so install the deb first.
$(MLNX_PLATFORM_API_RS)_DEPENDS   = $(LIBSWSSCOMMON_DEV)
SONIC_MAKE_FILES += $(MLNX_PLATFORM_API_RS)

# A docker image's _FILES are its prerequisites, so this makes an ordinary image
# build run the vendor tests too.  The pmon Dockerfile does not copy the file.
$(DOCKER_PLATFORM_MONITOR)_FILES += $(MLNX_PLATFORM_API_RS)

export MLNX_PLATFORM_API_RS
