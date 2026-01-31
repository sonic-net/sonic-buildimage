# SONIE Installer Image (.bin) rule

# Define the target based on the configured platform
SONIE_IMAGE = sonie-$(CONFIGURED_PLATFORM).bin

# The .bin target depends on the .efi target and has an explicit recipe.
$(TARGET_PATH)/$(SONIE_IMAGE): $(TARGET_PATH)/$(SONIE_UKI)
	@echo "--- Building SONIE Installer $(@) from payload $< ---"
	zip -j $(TARGET_PATH)/$(SONIE_UKI).zip $<
	IMAGE_TYPE=sonie \
	SONIE_INSTALLER_PAYLOAD=$(TARGET_PATH)/$(SONIE_UKI).zip \
	./build_image.sh $@
	rm -f $(TARGET_PATH)/$(SONIE_UKI).zip
