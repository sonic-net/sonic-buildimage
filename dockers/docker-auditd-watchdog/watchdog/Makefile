.ONESHELL:
SHELL = /bin/bash
.SHELLFLAGS += -e

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
