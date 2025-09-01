PWD		= $(shell pwd)
# Find the include directories in all subdirectories.
INCLUDE_DIRS := $(shell find $(PWD) -type d -name include)

# Add all include directories to EXTRA_CFLAGS.
EXTRA_CFLAGS := $(foreach dir,$(INCLUDE_DIRS),-I$(dir))

EXTRA_CFLAGS+= -Wall

MAKEFILE_FILE_PATH = $(abspath $(lastword $(MAKEFILE_LIST)))
BSP_KERNEL_DEBUG_PATH = $(abspath $(MAKEFILE_FILE_PATH)/../../public/include)

EXTRA_CFLAGS+= -I$(BSP_KERNEL_DEBUG_PATH)

ifdef ENABLE_GCOV
ifeq ($(ENABLE_GCOV), y)
EXTRA_CFLAGS+= -fprofile-arcs -ftest-coverage -lgcov
endif
endif # ENABLE_GCOV

KVERSION   ?= $(shell uname -r)
KERNEL_SRC ?=  /lib/modules/$(KVERSION)

module_out_put_dir := $(PWD)/build

obj-m += common/
ifeq ("$(CONFIGURED_ARCH)", "amd64")
obj-m += arch_x86/
endif
ifeq ("$(CONFIGURED_ARCH)", "arm64")
obj-m += arch_arm/
endif

all: build install
	@echo "build common modules success"

build:
	$(MAKE) -C $(KERNEL_SRC)/build M=$(PWD) EXTRA_CFLAGS="$(EXTRA_CFLAGS)"  modules

install:
	@if [ ! -d $(module_out_put_dir) ]; then mkdir -p $(module_out_put_dir) ;fi
	find $(PWD) -path $(module_out_put_dir) -prune -o -name "*.ko" -exec cp {} $(module_out_put_dir) \;

clean:
	$(MAKE) -C $(KERNEL_SRC)/build M=$(PWD) clean
	rm -rf $(module_out_put_dir)
	find . -name "*.o" -o -name "*.ko" -o "*.mod.c" \
	-o "*.cmd " -o "Module.markers" -o "Module.symvers" \
	-o "modules.order" -o ".tmp_versions" | xargs rm -f
