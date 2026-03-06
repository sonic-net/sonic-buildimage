.ONESHELL:
SHELL = /bin/bash
.SHELLFLAGS += -e

.PHONY: build test clean build-release test-release clean-release lint fmt

#
# Debug build targets
#
build:
	cargo build --all

test:
	cargo test --all

clean:
	cargo clean

#
# Release build targets
#
build-release:
	cargo build --release --all

test-release:
	cargo test --release --all

clean-release:
	cargo clean --release

#
# Lint and format targets (required by SONiC maintainers)
#
lint:
	cargo clippy -- -D warnings

fmt:
	cargo fmt --check
