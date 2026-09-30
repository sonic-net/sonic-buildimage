CC ?= $(CROSS)gcc
AR ?= $(CROSS)ar
AS ?= $(CROSS)as
LD ?= $(CROSS)ld
STRIP ?= $(CROSS)strip

install_root:=${top_srcdir}/images

install_header_dir:=${install_root}/header
install_adir:=$(install_root)/lib
install_symbol_dir:=$(install_root)/symbol
symbol_files:=$(shell find $(EXPORT_SYMBOL) -name 'Module.symvers')
#
# symbol_files += $(shell find $(install_symbol_dir) -name 'Module.symvers')
# KBUILD_EXTRA_SYMBOLS += $(symbol_files)
# export KBUILD_EXTRA_SYMBOLS

# top root: install_rootfs_dir
install_rootfs_dir:=$(install_root)/rootfs

install_sodir:=$(install_rootfs_dir)/$(INSTALL_SODIR)

install_usr_bin_dir:=$(install_rootfs_dir)/usr/bin
install_sbin_dir:=$(install_rootfs_dir)/sbin
install_etc_dir:=$(install_rootfs_dir)/etc

export INSTALL_MOD_PATH:=$(ROOT)

BUILD_CFLAGS:=$(CFLAGS) -I$(install_header_dir)
BUILD_LDFLAGS:=$(LDFLAGS) -L/$(install_sodir) -L/$(install_adir)

define compile_dirs
.PHONY: $(1)
$(1):
	@echo;echo "building $(1)..."
	@$(MAKE) -C ${1}
endef

compile.c = $(CC) $(BUILD_CFLAGS) -d -c -o $@ $<
%.o: %.c
	$(compile.c)

