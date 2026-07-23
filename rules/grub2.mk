# grub2 package
#
# resolute: SONiC does not customize grub itself — the historical patches only
# adjusted Debian packaging (debian/control, debian/rules) and skipped a cpio
# test, none of which touch the grub binaries. The rebuilt-from-source debs were
# therefore functionally identical to Ubuntu's own. Procure the official Ubuntu
# grub debs directly via SONIC_ONLINE_DEBS (like the kernel), instead of building
# from source. Versions are pinned for reproducibility; bump GRUB2_VERSION /
# GRUB2_UNSIGNED_VERSION here when moving to a newer archive build.
# Secure Boot is unaffected: signed shim/grub/mm come from the separate
# post-install signing flow, not from these unsigned debs.

GRUB2_VERSION := 2.14-2ubuntu2
GRUB2_UNSIGNED_VERSION := 2.14-2ubuntu1
export GRUB2_VERSION
export GRUB2_UNSIGNED_VERSION

GRUB2_POOL_URL := http://archive.ubuntu.com/ubuntu/pool/main/g/grub2
GRUB2_UNSIGNED_POOL_URL := http://archive.ubuntu.com/ubuntu/pool/main/g/grub2-unsigned

GRUB2_COMMON = grub2-common_$(GRUB2_VERSION)_$(CONFIGURED_ARCH).deb
$(GRUB2_COMMON)_URL = $(GRUB2_POOL_URL)/$(GRUB2_COMMON)

GRUB_COMMON = grub-common_$(GRUB2_VERSION)_$(CONFIGURED_ARCH).deb
$(GRUB_COMMON)_URL = $(GRUB2_POOL_URL)/$(GRUB_COMMON)

GRUB_EFI = grub-efi_$(GRUB2_VERSION)_$(CONFIGURED_ARCH).deb
$(GRUB_EFI)_URL = $(GRUB2_POOL_URL)/$(GRUB_EFI)

SONIC_ONLINE_DEBS += $(GRUB2_COMMON) $(GRUB_COMMON) $(GRUB_EFI)

ifeq ($(CONFIGURED_ARCH),amd64)
GRUB_PC_BIN = grub-pc-bin_$(GRUB2_VERSION)_$(CONFIGURED_ARCH).deb
$(GRUB_PC_BIN)_URL = $(GRUB2_POOL_URL)/$(GRUB_PC_BIN)
SONIC_ONLINE_DEBS += $(GRUB_PC_BIN)
endif

ifeq ($(CONFIGURED_ARCH),amd64)
GRUB_EFI_AMD64 = grub-efi-amd64_$(GRUB2_UNSIGNED_VERSION)_$(CONFIGURED_ARCH).deb
$(GRUB_EFI_AMD64)_URL = $(GRUB2_UNSIGNED_POOL_URL)/$(GRUB_EFI_AMD64)
GRUB_EFI_AMD64_BIN = grub-efi-amd64-bin_$(GRUB2_UNSIGNED_VERSION)_$(CONFIGURED_ARCH).deb
$(GRUB_EFI_AMD64_BIN)_URL = $(GRUB2_UNSIGNED_POOL_URL)/$(GRUB_EFI_AMD64_BIN)
SONIC_ONLINE_DEBS += $(GRUB_EFI_AMD64) $(GRUB_EFI_AMD64_BIN)
GRUB_EFI_MAIN = $(GRUB_EFI_AMD64)
else ifeq ($(CONFIGURED_ARCH),arm64)
GRUB_EFI_ARM64 = grub-efi-arm64_$(GRUB2_UNSIGNED_VERSION)_$(CONFIGURED_ARCH).deb
$(GRUB_EFI_ARM64)_URL = $(GRUB2_UNSIGNED_POOL_URL)/$(GRUB_EFI_ARM64)
GRUB_EFI_ARM64_BIN = grub-efi-arm64-bin_$(GRUB2_UNSIGNED_VERSION)_$(CONFIGURED_ARCH).deb
$(GRUB_EFI_ARM64_BIN)_URL = $(GRUB2_UNSIGNED_POOL_URL)/$(GRUB_EFI_ARM64_BIN)
SONIC_ONLINE_DEBS += $(GRUB_EFI_ARM64) $(GRUB_EFI_ARM64_BIN)
GRUB_EFI_MAIN = $(GRUB_EFI_ARM64)
endif
