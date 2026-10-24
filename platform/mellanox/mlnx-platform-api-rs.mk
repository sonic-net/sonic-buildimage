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


# The native NVIDIA platform API, in Rust: mlnx-platform-api-rs.
#
# Not a package of its own.  It is the crate the Rust pmon daemons link as
# `platform-native` when they are built for this platform, in place of the stub
# sonic-platform-common carries -- and choosing the cargo config below is the
# whole of that switch; see mlnx-platform-api-rs/platform-native.cargo.toml
# and platform-api-rs-crates/README.md.  A daemon runs it when its
# pmon_daemon_control.json says "platform_api_<daemon>": "native", which every
# NVIDIA device that runs a Rust daemon does; "pyo3" (or no key) puts that
# daemon back on the Python platform API.
#
# platform/mellanox/rules.mk is read after rules/*.mk, so this assignment
# replaces the generic config rules/sonic-pmon-daemons-rs.mk set.
$(SONIC_THERMALCTLD_RS)_BUILD_ENV = SONIC_CARGO_CONFIG=$(CURDIR)/$(PLATFORM_PATH)/mlnx-platform-api-rs/platform-native.cargo.toml

# The crate's own unit tests.  The daemons' build compiles the crate but runs
# only the daemons' tests, so without this target nothing would run its own:
#
#     make target/files/$(BLDENV)/mlnx-platform-api-rs
#
# The artifact is a receipt that the tests passed; nothing consumes it.
MLNX_PLATFORM_API_RS = mlnx-platform-api-rs
$(MLNX_PLATFORM_API_RS)_SRC_PATH = $(PLATFORM_PATH)/mlnx-platform-api-rs
SONIC_MAKE_FILES += $(MLNX_PLATFORM_API_RS)

# A docker image's _FILES are its prerequisites, so an ordinary pmon image
# build runs the crate's tests too.  The pmon Dockerfile does not copy the file.
$(DOCKER_PLATFORM_MONITOR)_FILES += $(MLNX_PLATFORM_API_RS)

export MLNX_PLATFORM_API_RS
