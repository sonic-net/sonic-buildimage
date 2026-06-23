# SONIE Installer Image (.bin) rule

# Define the target based on the configured platform
ifeq ($(CONFIGURED_ARCH),amd64)
SONIE_IMAGE = sonie-$(CONFIGURED_PLATFORM).bin
else
SONIE_IMAGE = sonie-$(CONFIGURED_PLATFORM)-$(CONFIGURED_ARCH).bin
endif

# The .bin target depends on the .efi target and has an explicit recipe.
$(TARGET_PATH)/$(SONIE_IMAGE): $(TARGET_PATH)/$(SONIE_UKI)
	IMAGE_TYPE=sonie \
	PAYLOAD_UKI=$(TARGET_PATH)/$(SONIE_UKI) \
	FILESYSTEM_ROOT=fsroot-$(CONFIGURED_PLATFORM)-recovery \
	./build_image.sh $@

# Alias for .bin target without architecture suffix
ifneq ($(CONFIGURED_ARCH),amd64)
ifneq ($(CONFIGURED_ARCH),arm64)
$(TARGET_PATH)/sonie-$(CONFIGURED_PLATFORM).bin: $(TARGET_PATH)/$(SONIE_IMAGE)
endif
endif
