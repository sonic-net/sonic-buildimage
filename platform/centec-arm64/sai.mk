# Centec SAI

export CENTEC_SAI_VERSION = 1.10.2-1
#export CENTEC_SAI = libsaictc_$(CENTEC_SAI_VERSION)_$(PLATFORM_ARCH).deb
#export CENTEC_SAI_DEV = libsaictc-dev_$(CENTEC_SAI_VERSION)_$(PLATFORM_ARCH).deb
#export CENTEC_SAI_URL_PREFIX = "https://github.com/CentecNetworks/sonic-binaries/raw/master/$(PLATFORM_ARCH)/sai"
export CENTEC_SAI = libsai_$(CENTEC_SAI_VERSION)_$(PLATFORM_ARCH).deb

$(CENTEC_SAI)_URL = https://raw.githubusercontent.com/philo-micas/sonic-binaries/main/centec-arm64/sai/$(CENTEC_SAI)

#$(CENTEC_SAI)_URL = $(CENTEC_SAI_URL_PREFIX)/$(CENTEC_SAI)
#$(CENTEC_SAI_DEV)_URL = $(CENTEC_SAI_URL_PREFIX)/$(CENTEC_SAI_DEV)
#$(eval $(call add_conflict_package,$(CENTEC_SAI_DEV),$(LIBSAIVS_DEV)))

SONIC_ONLINE_DEBS += $(CENTEC_SAI)
#SONIC_ONLINE_DEBS += $(CENTEC_SAI_DEV)
#$(CENTEC_SAI_DEV)_DEPENDS += $(CENTEC_SAI)
