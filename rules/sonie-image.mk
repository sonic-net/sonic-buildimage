# SONiE Installer Image (.bin) rule

# Define the target based on the configured platform
SONIE_IMAGE = sonie-$(CONFIGURED_PLATFORM).bin

DEP_FILES = $(TARGET_PATH)/$(SONIE_UKI)

# The .bin target depends on the .efi target and has an explicit recipe.
$(TARGET_PATH)/$(SONIE_IMAGE): $(TARGET_PATH)/$(SONIE_UKI)
	@echo "--- Building SONiE Installer $(@) from payload $< ---"
	IMAGE_TYPE=sonie \
	SONIE_INSTALLER_PAYLOAD=$< \
	./build_image.sh $@
