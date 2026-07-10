include ../../rules/sonic-nettools.mk

$(addprefix $(DEST)/, $(SONIC_NETTOOLS)): $(DEST)/% :
	mkdir -p bin
ifeq ($(CROSS_BUILD_ENVIRON), y)
	cargo test --target=$(RUST_CROSS_COMPILE_TARGET)
	cargo build --release --target=$(RUST_CROSS_COMPILE_TARGET)
	mv -f target/$(RUST_CROSS_COMPILE_TARGET)/release/wol bin/
else
	cargo test
	cargo build --release
	mv -f target/release/wol bin/
endif

clean:
	rm -rf target
	rm -rf bin

test:
	cargo test

fmt:
	cargo clippy
