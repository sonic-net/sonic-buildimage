.ONESHELL:
SHELL = /bin/bash
.SHELLFLAGS += -e

#
# Top level targets
#
all: format lint build test
release: format lint build-release test-release

#
# Format tasks
#
format:
	cargo fmt -- --emit files

#
# Lint tasks
#
pre-commit:
	pre-commit run --all-files

lint:
	cargo clippy --all-targets --all-features

lint-fix:
	cargo clippy --all-targets --all-features --fix --allow-dirty

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

ci-all: | ci-format ci-build ci-doc ci-lint ci-test

ci-format:
	cargo fmt --check --all

ci-build:
	RUSTFLAGS="--deny warnings" cargo build           --workspace --all-features
	RUSTFLAGS="--deny warnings" cargo build --release --workspace --all-features

ci-doc:
	RUSTDOCFLAGS="--deny warnings" cargo doc           --workspace --all-features
	RUSTDOCFLAGS="--deny warnings" cargo doc --release --workspace --all-features

ci-lint:
	cargo clippy           --workspace --all-features --no-deps -- --deny "clippy::all"
	cargo clippy --release --workspace --all-features --no-deps -- --deny "clippy::all"

ci-test:
	cargo test           --workspace --all-features
	cargo test --release --workspace --all-features
