TARGET_ARCH := $(or $(CONFIGURED_ARCH),$(DEB_HOST_ARCH),$(shell dpkg-architecture -qDEB_HOST_ARCH))
RUST_TARGET ?= $(strip \
	$(if $(filter amd64,$(TARGET_ARCH)),x86_64-unknown-linux-gnu, \
	$(if $(filter arm64,$(TARGET_ARCH)),aarch64-unknown-linux-gnu, \
	$(if $(filter armhf,$(TARGET_ARCH)),armv7-unknown-linux-gnueabihf))))
ifeq ($(RUST_TARGET),)
$(error Unsupported Rust target architecture: $(TARGET_ARCH))
endif
CARGO_TARGET_DIR ?= target
CARGO_FLAGS = --locked --target $(RUST_TARGET)

.PHONY: all build test install clean

all: build

build:
	cargo build --release $(CARGO_FLAGS)

test:
	cargo fmt --check --all
	cargo metadata --locked --all-features --filter-platform $(RUST_TARGET) > /dev/null
	cargo clippy --workspace --locked --all-targets --all-features --target $(RUST_TARGET) -- -D warnings
	cargo test --workspace --locked --all-targets --all-features --target $(RUST_TARGET)

install: build
	install -D -m 0755 $(CARGO_TARGET_DIR)/$(RUST_TARGET)/release/sonic-cfggen-lite-rs $(DESTDIR)/usr/bin/sonic-cfggen-lite-rs

clean:
	cargo clean
