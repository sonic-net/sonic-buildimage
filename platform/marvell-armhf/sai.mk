# Marvell SAI

export MRVL_SAI_VERSION = 1.12.0-1
export MRVL_SAI = mrvllibsai_$(MRVL_SAI_VERSION)_$(PLATFORM_ARCH).deb

$(MRVL_SAI)_SRC_PATH = $(PLATFORM_PATH)/sai
$(eval $(call add_conflict_package,$(MRVL_SAI),$(LIBSAIVS_DEV)))

SONIC_MAKE_DEBS += $(MRVL_SAI)
