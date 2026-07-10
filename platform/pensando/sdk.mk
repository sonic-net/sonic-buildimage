# Pensando SAI
PENSANDO_SAI = libsai_1.10.1-0_arm64.deb
PENSANDO_SAI_DEV = libsai-dev_1.10.1-0_arm64.deb
$(PENSANDO_SAI)_PATH = $(PLATFORM_PATH)/pensando-sonic-artifacts
$(PENSANDO_SAI_DEV)_PATH = $(PLATFORM_PATH)/pensando-sonic-artifacts

$(eval $(call add_conflict_package,$(PENSANDO_SAI_DEV),$(LIBSAIVS_DEV)))

SONIC_COPY_DEBS += $(PENSANDO_SAI)
SONIC_COPY_DEBS += $(PENSANDO_SAI_DEV)
