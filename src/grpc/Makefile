SHELL = /bin/bash
.ONESHELL:
.SHELLFLAGS += -e

MAIN_TARGET = libgrpc10_$(GRPC_VERSION_FULL)_$(CONFIGURED_ARCH).deb
DERIVED_TARGETS = libgrpc-dev_$(GRPC_VERSION_FULL)_$(CONFIGURED_ARCH).deb \
					libgrpc++1_$(GRPC_VERSION_FULL)_$(CONFIGURED_ARCH).deb \
					libgrpc++-dev_$(GRPC_VERSION_FULL)_$(CONFIGURED_ARCH).deb \
					python3-grpcio_$(GRPC_VERSION_FULL)_$(CONFIGURED_ARCH).deb \
					protobuf-compiler-grpc_$(GRPC_VERSION_FULL)_$(CONFIGURED_ARCH).deb

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	# Remove any stale files
	rm -rf grpc-$(GRPC_VERSION)

	dget -u grpc_$(GRPC_VERSION_FULL).dsc https://deb.debian.org/debian/pool/main/g/grpc/grpc_$(GRPC_VERSION_FULL).dsc

	pushd grpc-$(GRPC_VERSION)

ifeq ($(CROSS_BUILD_ENVIRON), y)
	dpkg-buildpackage -us -uc -b -a$(CONFIGURED_ARCH) -Pcross,nocheck -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
else
	dpkg-buildpackage -us -uc -b -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
endif

	popd

	mv $(DERIVED_TARGETS) $* $(DEST)/

$(addprefix $(DEST)/, $(DERIVED_TARGETS)): $(DEST)/% : $(DEST)/$(MAIN_TARGET)