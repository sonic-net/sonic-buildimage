.ONESHELL:
SHELL = /bin/bash
.SHELLFLAGS += -e

all: format lint build test

format:
	cargo fmt --all

lint:
	cargo clippy --workspace --all-targets -- --deny warnings

build:
	cargo build --release --workspace

test:
	cargo test --workspace

clean:
	cargo clean
