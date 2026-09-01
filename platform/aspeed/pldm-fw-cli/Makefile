# Build pldm-fw (CodeConstruct mctp-rs pldm-fw-cli) and pack as a .deb.

.ONESHELL:
SHELL = /bin/bash

MAIN_TARGET = $(PLDM_FW)
DERIVED_TARGETS =

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	set -e
	rm -rf upstream-ws upstream.tar.gz stage-deb
	wget "$(PLDM_FW_ARCHIVE_URL)" -O upstream.tar.gz
	# Verify the download against the pinned checksum before extracting, so a
	# corrupted or tampered tarball aborts the build (fail closed).
	if [ -z "$(PLDM_FW_ARCHIVE_SHA256)" ]; then
		echo "ERROR: PLDM_FW_ARCHIVE_SHA256 is unset; refusing to extract unverified $(PLDM_FW_ARCHIVE_URL)" >&2
		exit 1
	fi
	echo "$(PLDM_FW_ARCHIVE_SHA256)  upstream.tar.gz" | sha256sum -c -
	mkdir -p upstream-ws
	tar -C upstream-ws --strip-components=1 -xf upstream.tar.gz
	mkdir -p stage-deb/DEBIAN stage-deb/usr/bin
	pushd upstream-ws
	if [ -n "$$RUST_CROSS_COMPILE_TARGET" ]; then
		_u=$$(echo "$$RUST_CROSS_COMPILE_TARGET" | tr 'a-z-' 'A-Z_')
		export CARGO_TARGET_$${_u}_LINKER="$(CC)"
		cargo build --release -p pldm-fw-cli --target "$$RUST_CROSS_COMPILE_TARGET"
		BIN="target/$$RUST_CROSS_COMPILE_TARGET/release/pldm-fw"
	else
		cargo build --release -p pldm-fw-cli
		BIN="target/release/pldm-fw"
	fi
	test -f "$$BIN"
	install -D -m755 "$$BIN" ../stage-deb/usr/bin/pldm-fw
	popd
	{ \
	echo "Package: pldm-fw"; \
	echo "Version: $(PLDM_FW_PKG_VERSION)"; \
	echo "Section: utils"; \
	echo "Priority: optional"; \
	echo "Architecture: $(CONFIGURED_ARCH)"; \
	echo "Maintainer: SONiC <sonic@microsoft.com>"; \
	echo "Depends: libc6 (>= 2.31), mctp"; \
	echo "Description: PLDM for Firmware Update CLI (CodeConstruct mctp-rs)"; \
	echo " pldm-fw from mctp-rs pldm-fw-cli; PLDM type 5 UA over MCTP."; \
	} > stage-deb/DEBIAN/control
	fakeroot dpkg-deb --root-owner-group -Zxz -b stage-deb "$(DEST)/$(PLDM_FW)"
	rm -rf upstream-ws upstream.tar.gz stage-deb
