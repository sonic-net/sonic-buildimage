# grub2 package

GRUB2_VERSION := 2.06-13+deb13u1

export GRUB2_VERSION

GRUB2_COMMON = grub2-common_$(GRUB2_VERSION)_$(CONFIGURED_ARCH).deb
$(GRUB2_COMMON)_SRC_PATH = $(SRC_PATH)/grub2
SONIC_MAKE_DEBS += $(GRUB2_COMMON)

GRUB_COMMON = grub-common_$(GRUB2_VERSION)_$(CONFIGURED_ARCH).deb
$(eval $(call add_derived_package,$(GRUB2_COMMON),$(GRUB_COMMON)))

GRUB_EFI = grub-efi_$(GRUB2_VERSION)_$(CONFIGURED_ARCH).deb
$(eval $(call add_derived_package,$(GRUB2_COMMON),$(GRUB_EFI)))

ifeq ($(CONFIGURED_ARCH),amd64)
GRUB_PC_BIN = grub-pc-bin_$(GRUB2_VERSION)_$(CONFIGURED_ARCH).deb
$(eval $(call add_derived_package,$(GRUB2_COMMON),$(GRUB_PC_BIN)))

GRUB_EFI_AMD64 = grub-efi-amd64_$(GRUB2_VERSION)_$(CONFIGURED_ARCH).deb
$(eval $(call add_derived_package,$(GRUB2_COMMON),$(GRUB_EFI_AMD64)))

GRUB_EFI_AMD64_BIN = grub-efi-amd64-bin_$(GRUB2_VERSION)_$(CONFIGURED_ARCH).deb
$(eval $(call add_derived_package,$(GRUB2_COMMON),$(GRUB_EFI_AMD64_BIN)))
else ifeq ($(CONFIGURED_ARCH),arm64)
GRUB_EFI_ARM64 = grub-efi-arm64_$(GRUB2_VERSION)_$(CONFIGURED_ARCH).deb
$(eval $(call add_derived_package,$(GRUB2_COMMON),$(GRUB_EFI_ARM64)))

GRUB_EFI_ARM64_BIN = grub-efi-arm64-bin_$(GRUB2_VERSION)_$(CONFIGURED_ARCH).deb
$(eval $(call add_derived_package,$(GRUB2_COMMON),$(GRUB_EFI_ARM64_BIN)))
endif

# The .c, .cpp, .h & .hpp files under src/{$DBG_SRC_ARCHIVE list}
# are archived into debug one image to facilitate debugging.
#
DBG_SRC_ARCHIVE += openssh
