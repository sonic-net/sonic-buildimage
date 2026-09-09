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

#
# Developer/CI checks for both Rust crates under platform/mellanox.  Not part of
# the build above - nothing in the image depends on them - so they can be run on
# a workstation without a DEST.  Target names match sonic-dash-ha's, which is the
# only other Rust in SONiC.
#
#   make ci-all
#
HW_MGMT = ../hw-management-rs/Cargo.toml

.PHONY: ci-all ci-format ci-lint ci-build ci-doc ci-test format

ci-all: ci-format ci-lint ci-build ci-doc ci-test

# platform/mellanox/rustfmt.toml is the policy for both crates; rustfmt finds it
# by walking up from each source file.
ci-format:
	cargo fmt --check --all
	cargo fmt --check --all --manifest-path $(HW_MGMT)

# clippy::all denied, not warned.  Needs libswsscommon-dev, which the slave
# image the SONiC build runs in already carries.
ci-lint:
	cargo clippy --all-targets --all-features --no-deps -- --deny "clippy::all"
	cargo clippy --all-targets --all-features --no-deps --manifest-path $(HW_MGMT) -- --deny "clippy::all"

# The fixer, for local use.  CI never runs this one.
format:
	cargo fmt --all
	cargo fmt --all --manifest-path $(HW_MGMT)

# Warnings are errors.  Debug and release both, because a cfg or a lint can
# differ between the two and what ships is the release one.
#
# sonic-dash-ha runs `cargo clean` between the two passes; that is a disk-space
# measure for its agent pool, and it would throw away a contributor's build
# cache.  Omitted: the two use separate target directories.
ci-build:
	RUSTFLAGS="--deny warnings" cargo build --all-features
	RUSTFLAGS="--deny warnings" cargo build --all-features --release
	RUSTFLAGS="--deny warnings" cargo build --all-features --manifest-path $(HW_MGMT)
	RUSTFLAGS="--deny warnings" cargo build --all-features --release --manifest-path $(HW_MGMT)

# A broken intra-doc link is a broken link whether or not anything reads it.
ci-doc:
	RUSTDOCFLAGS="--deny warnings" cargo doc --all-features --no-deps
	RUSTDOCFLAGS="--deny warnings" cargo doc --all-features --no-deps --manifest-path $(HW_MGMT)

# The release pass is what the build rule above already runs; the debug pass is
# added here so a failure that only shows without optimisation is not missed.
ci-test:
	cargo test --all-features
	cargo test --all-features --release
	cargo test --all-features --manifest-path $(HW_MGMT)
	cargo test --all-features --release --manifest-path $(HW_MGMT)
