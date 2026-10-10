# Centec DAL

export CENTEC_DAL_VERSION = 1.0+6.12.41+deb13-sonic
export CENTEC_DAL = centec-dal_$(CENTEC_DAL_VERSION)_$(PLATFORM_ARCH).deb
export CENTEC_DAL_URL_PREFIX = "https://github.com/CentecNetworks/sonic-binaries/raw/master/$(PLATFORM_ARCH)/sai/SONIC202511"

$(CENTEC_DAL)_URL = $(CENTEC_DAL_URL_PREFIX)/$(CENTEC_DAL)
SONIC_ONLINE_DEBS += $(CENTEC_DAL)
