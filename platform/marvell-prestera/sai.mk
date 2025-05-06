# Marvell SAI

BRANCH = master
ifeq ($(CONFIGURED_ARCH),arm64)
MRVL_SAI_VERSION = 1.15.1-1
else ifeq ($(CONFIGURED_ARCH),armhf)
MRVL_SAI_VERSION = 1.15.1-1
else
MRVL_SAI_VERSION = 1.15.1-1
endif

MRVL_SAI_URL_PREFIX = https://github.com/Marvell-switching/sonic-marvell-binaries/raw/master/$(CONFIGURED_ARCH)/sai-plugin/$(BRANCH)/
MRVL_SAI = mrvllibsai_$(MRVL_SAI_VERSION)_$(PLATFORM_ARCH).deb
$(MRVL_SAI)_URL = $(MRVL_SAI_URL_PREFIX)/$(MRVL_SAI)

SONIC_ONLINE_DEBS += $(MRVL_SAI)
$(MRVL_SAI)_SKIP_VERSION=y
$(eval $(call add_conflict_package,$(MRVL_SAI),$(LIBSAIVS_DEV)))

