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

.ONESHELL:
SHELL = /bin/bash
.SHELLFLAGS += -e

MAIN_TARGET = mlnx-platform-api-rs

# Build and unit-test the Rust platform API on its own.  .SHELLFLAGS carries
# -e, so a failing test fails the build.  The artifact is a receipt that the
# tests ran; nothing consumes it - the daemon compiles this crate from source.
$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	cargo test --release
	cargo test --release --manifest-path ../hw-management-rs/Cargo.toml
	cargo build --release
	echo "mlnx-platform-api-rs: tests passed" > $(DEST)/$(MAIN_TARGET)
