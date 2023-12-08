# Pensando SAI
PENSANDO_SAI = libsai_1.10.1-0_arm64.deb
PENSANDO_SAI_DEV = libsai-dev_1.10.1-0_arm64.deb
$(PENSANDO_SAI)_URL = https://github.com/pensando/dsc-artifacts/blob/main/libsai_1.10.1-0_arm64.deb?raw=true
$(PENSANDO_SAI_DEV)_URL = https://github.com/pensando/dsc-artifacts/blob/main/libsai-dev_1.10.1-0_arm64.deb?raw=true

$(eval $(call add_conflict_package,$(PENSANDO_SAI_DEV),$(LIBSAIVS_DEV)))

SONIC_ONLINE_DEBS += $(PENSANDO_SAI)
SONIC_ONLINE_DEBS += $(PENSANDO_SAI_DEV)
