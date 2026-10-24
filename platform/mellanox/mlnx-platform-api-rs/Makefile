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

# Built through ../platform-api-rs-crates/platform-native, the path the
# daemons' build reaches this crate by: its path dependencies name its siblings
# there.  The hw-mgmt crate it depends on is built the same way, and tested
# with it.
MANIFEST = ../platform-api-rs-crates/platform-native/Cargo.toml
HW_MGMT_MANIFEST = ../platform-api-rs-crates/hw-mgmt/Cargo.toml

# Without $PLATFORM.  Makefile.work runs slave.mk with PLATFORM=<platform> on
# its command line, which make exports to every recipe, and platform-base's
# get_platform() reads $PLATFORM before machine.conf: every test that sets a
# platform up through machine.conf would skip, and the receipt would say they
# passed.
TEST = env -u PLATFORM cargo test

# Build and unit-test the crate on its own.  .SHELLFLAGS carries -e, so a
# failing test fails the build.
$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	$(TEST) --release --manifest-path $(HW_MGMT_MANIFEST)
	$(TEST) --release --manifest-path $(MANIFEST)
	echo "mlnx-platform-api-rs: tests passed" > $(DEST)/$(MAIN_TARGET)

# Developer checks, not part of the build above.
.PHONY: ci-lint ci-test
ci-lint:
	cargo clippy --all-targets --manifest-path $(HW_MGMT_MANIFEST) -- --deny warnings
	cargo clippy --all-targets --manifest-path $(MANIFEST) -- --deny warnings
ci-test:
	$(TEST) --manifest-path $(HW_MGMT_MANIFEST)
	$(TEST) --release --manifest-path $(HW_MGMT_MANIFEST)
	$(TEST) --manifest-path $(MANIFEST)
	$(TEST) --release --manifest-path $(MANIFEST)
