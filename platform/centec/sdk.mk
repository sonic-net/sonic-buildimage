# Centec SAI
CENTEC_SAI = libsai_1.9.1-1_amd64.deb
$(CENTEC_SAI)_URL = https://github.com/CentecNetworks/sonic-binaries/raw/master/amd64/sai/$(CENTEC_SAI)

CENTEC_SAI_DEV = libsai-dev_1.9.1-1_amd64.deb
$(CENTEC_SAI_DEV)_URL = https://github.com/CentecNetworks/sonic-binaries/raw/master/amd64/sai/$(CENTEC_SAI_DEV)
$(eval $(call add_conflict_package,$(CENTEC_SAI_DEV),$(LIBSAIVS_DEV)))

SONIC_ONLINE_DEBS += $(CENTEC_SAI)
SONIC_ONLINE_DEBS += $(CENTEC_SAI_DEV)
