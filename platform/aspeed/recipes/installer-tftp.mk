# Aspeed TFTP / U-Boot netboot installer bundle (FIT + payload).
#
# Platform-local installer recipes and scripts live under platform/aspeed/installer/.
#
# This is not added as a second SONIC_INSTALLERS target: that would register another RFS squashfs
# name and rebuild the same rootfs (slave.mk rfs_define_target). The bundle reuses fsroot-aspeed
# produced for sonic-aspeed-arm64.bin.

SONIC_ASPEED_TFTP_TAR = sonic-aspeed-arm64-tftp-install.tar

ASPEED_EMMC_IMAGE_STEM = sonic-aspeed-arm64-emmc
ASPEED_EMMC_IMAGE_GZ = $(TARGET_PATH)/$(ASPEED_EMMC_IMAGE_STEM).img.gz
ASPEED_TFTP_BUILD_DIR = $(TARGET_PATH)/aspeed-tftp

$(ASPEED_EMMC_IMAGE_GZ): $(TARGET_PATH)/$(SONIC_ONE_IMAGE) $(PLATFORM_PATH)/onie-image-arm64.conf
	@echo "=== Building $(ASPEED_EMMC_IMAGE_GZ) (sudo required) ==="
	sudo $(PLATFORM_PATH)/build-emmc-image-installer.sh \
		$(abspath $(TARGET_PATH)/$(SONIC_ONE_IMAGE)) \
		$(abspath $(TARGET_PATH)/$(ASPEED_EMMC_IMAGE_STEM).img)
	@test -f $@ || { echo "Error: expected $@ after build-emmc-image-installer.sh"; exit 1; }

.PHONY: aspeed-emmc-image
aspeed-emmc-image: $(ASPEED_EMMC_IMAGE_GZ)

$(ASPEED_TFTP_BUILD_DIR)/.stamp: $(TARGET_PATH)/$(SONIC_ONE_IMAGE) $(ASPEED_EMMC_IMAGE_GZ)
	$(PLATFORM_PATH)/installer/create_tftp_image.sh $(abspath $(ASPEED_TFTP_BUILD_DIR))
	@touch $@

aspeed-tftp: $(ASPEED_TFTP_BUILD_DIR)/.stamp
.PHONY: aspeed-tftp

$(TARGET_PATH)/$(SONIC_ASPEED_TFTP_TAR): $(TARGET_PATH)/$(SONIC_ONE_IMAGE) $(ASPEED_EMMC_IMAGE_GZ)
	OUTPUT_TFTP_INSTALL_TAR=$(abspath $@) $(PLATFORM_PATH)/installer/create_tftp_image.sh $(abspath $(ASPEED_TFTP_BUILD_DIR))

aspeed-tftp-tar: $(TARGET_PATH)/$(SONIC_ASPEED_TFTP_TAR)
.PHONY: aspeed-tftp-tar
