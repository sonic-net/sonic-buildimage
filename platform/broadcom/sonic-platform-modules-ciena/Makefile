# SPDX-License-Identifier: GPL-2.0
#
# Top-level Makefile for Ciena Platform Modules
# Builds common modules + platform-specific modules
#

# Kernel build environment
KERNEL_SRC ?= /lib/modules/$(shell uname -r)/build

# Platform selection
# Set PLATFORMS to build specific platforms, or leave empty to build all
# Examples:
#   make PLATFORMS="8140"      # Build CN8140 only
#   make PLATFORMS=""               # Build all platforms (default)
PLATFORMS ?= 8140

# Directories
COMMON_DIR := common
PLATFORM_DIRS := $(addprefix ,$(PLATFORMS))

.PHONY: all common $(PLATFORMS) clean install help

all: common $(PLATFORMS)
	@echo ""
	@echo "============================================"
	@echo "Build complete!"
	@echo "============================================"
	@echo "Common modules built in: $(COMMON_DIR)/"
	@echo "Platform modules built in: $(PLATFORMS)"
	@echo ""
	@echo "Modules built:"
	@echo "  Common: i2c-ciena, i2c-ciena-smb, i2c-fpga-mux,"
	@echo "          ciena_cic_trace, ciena_thermal, ciena_fan,"
	@echo "          sirilx_gpio, sirilx_uio, reset-*, led-sysfs, etc."
	@for platform in $(PLATFORMS); do \
		if [ "$$platform" = "8140" ]; then echo "  CN8140: rudra40_cic"; fi; \
	done
	@echo ""
	@echo "To install: make install"
	@echo "To clean:   make clean"

common:
	@echo ""
	@echo "============================================"
	@echo "Building common modules..."
	@echo "============================================"
	$(MAKE) -C $(COMMON_DIR) KERNEL_SRC=$(KERNEL_SRC)

8140: common
	@echo ""
	@echo "============================================"
	@echo "Building CN8140 (Rudra40) platform modules..."
	@echo "============================================"
	$(MAKE) -C 8140/modules KERNEL_SRC=$(KERNEL_SRC)

clean:
	@echo "Cleaning all modules..."
	$(MAKE) -C $(COMMON_DIR) clean
	@for platform in $(PLATFORMS); do \
		echo "Cleaning $$platform..."; \
		$(MAKE) -C $$platform/modules clean; \
	done
	@echo "Clean complete."

install: all
	@echo ""
	@echo "============================================"
	@echo "Installing modules..."
	@echo "============================================"
	$(MAKE) -C $(COMMON_DIR) install
	@for platform in $(PLATFORMS); do \
		echo "Installing $$platform modules..."; \
		$(MAKE) -C $$platform/modules install; \
	done
	@echo ""
	@echo "============================================"
	@echo "Installation complete!"
	@echo "============================================"
	@echo "Run 'depmod' to update module dependencies"

help:
	@echo "Ciena Platform Modules Build System"
	@echo "===================================="
	@echo ""
	@echo "Targets:"
	@echo "  all         - Build common + all platform modules"
	@echo "  common      - Build only common modules"
	@echo "  8140        - Build CN8140 (Rudra40) modules"
	@echo "  clean       - Clean all build artifacts"
	@echo "  install     - Install all modules to system"
	@echo "  help        - Show this help message"
	@echo ""
	@echo "Variables:"
	@echo "  KERNEL_SRC  - Kernel source directory"
	@echo "                (default: /lib/modules/\$$(uname -r)/build)"
	@echo "  PLATFORMS   - Space-separated list of platforms to build"
	@echo "                (default: 8140)"
	@echo ""
	@echo "Examples:"
	@echo "  make all                          - Build all platforms"
	@echo "  make PLATFORMS=\"8140\"             - Build CN8140 only"
	@echo "  make 8140                         - Build CN8140 explicitly"
	@echo "  make KERNEL_SRC=/usr/src/linux    - Build with custom kernel"
	@echo "  make clean                        - Clean everything"
	@echo "  make install                      - Install to /lib/modules"
	@echo ""
	@echo "Module Loading Order (Example for CN8140):"
	@echo "  1. modprobe uio"
	@echo "  2. modprobe regmap-sysfs"
	@echo "  3. modprobe ciena_cic_trace"
	@echo "  4. modprobe rudra40_cic"
	@echo "  5. modprobe reset-fpga reset-gpio reset-ipmi reset-sysfs"
	@echo "  6. modprobe sirilx_uio"
	@echo "  7. modprobe i2c-ciena i2c-ciena-smb i2c-fpga-mux"
	@echo "  8. modprobe i2c-dev"
	@echo ""
	@echo "Platform Status:"
	@echo "  CN8140 (Rudra40) - ✅ WORKING"
