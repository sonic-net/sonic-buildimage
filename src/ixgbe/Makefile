.ONESHELL:
SHELL = /bin/bash
.SHELLFLAGS += -e

MAIN_TARGET = ixgbe.ko

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	rm -rf ./ixgbe-$(IXGBE_DRIVER_VERSION)
	wget -O ixgbe-$(IXGBE_DRIVER_VERSION).tar.gz "$(BUILD_PUBLIC_URL)/ixgbe-5.2.4.tar.gz"
	tar xzf ixgbe-$(IXGBE_DRIVER_VERSION).tar.gz

	# Patch
	pushd ./ixgbe-$(IXGBE_DRIVER_VERSION)

	# Build the package
	pushd src
	export BUILD_KERNEL=$(KVERSION)
	make
	popd

	popd
	mv ./ixgbe-$(IXGBE_DRIVER_VERSION)/src/$* $(DEST)/
