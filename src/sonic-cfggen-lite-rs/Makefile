RUST_TARGET ?= $(strip $(or $(RUST_CROSS_COMPILE_TARGET),$(CARGO_BUILD_TARGET)))
ifeq ($(CROSS_BUILD_ENVIRON),y)
ifeq ($(strip $(RUST_TARGET)),)
$(error RUST_CROSS_COMPILE_TARGET must be set for cross builds)
endif
endif
CARGO_TARGET_DIR ?= target
CARGO_FLAGS = --locked $(if $(RUST_TARGET),--target $(RUST_TARGET))
CARGO_RELEASE_DIR = $(CARGO_TARGET_DIR)/$(if $(RUST_TARGET),$(RUST_TARGET)/)release

.PHONY: all build test install clean

all: build

build:
	cargo build --release $(CARGO_FLAGS)

test:
	cargo fmt --check --all
	cargo metadata --locked --all-features $(if $(RUST_TARGET),--filter-platform $(RUST_TARGET)) > /dev/null
	cargo clippy --workspace $(CARGO_FLAGS) --all-targets --all-features -- -D warnings
	cargo test --workspace $(CARGO_FLAGS) --all-targets --all-features

install: build
	install -D -m 0755 $(CARGO_RELEASE_DIR)/sonic-cfggen-lite-rs $(DESTDIR)/usr/bin/sonic-cfggen-lite-rs

clean:
	cargo clean
