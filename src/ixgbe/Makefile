.ONESHELL:
SHELL = /bin/bash
.SHELLFLAGS += -e

MAIN_TARGET = ixgbe.ko

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	rm -rf ./ixgbe-$(IXGBE_DRIVER_VERSION)
	wget -O ixgbe-$(IXGBE_DRIVER_VERSION).tar.gz "https://sourceforge.net/projects/e1000/files/ixgbe%20stable/5.19.9/ixgbe-5.19.9.tar.gz/download"
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
