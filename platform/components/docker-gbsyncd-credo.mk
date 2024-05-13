DOCKER_GBSYNCD_PLATFORM_CODE = credo

LIBSAI_CREDO = libsaicredo_0.9.6_amd64.deb
$(LIBSAI_CREDO)_URL = "https://sonicstorage.blob.core.windows.net/public/credosai/libsaicredo_0.9.6_amd64.deb"
LIBSAI_CREDO_OWL = libsaicredo-owl_0.9.6_amd64.deb
$(LIBSAI_CREDO_OWL)_URL = "https://sonicstorage.blob.core.windows.net/public/credosai/libsaicredo-owl_0.9.6_amd64.deb"
LIBSAI_CREDO_BLACKHAWK = libsaicredo-blackhawk_0.9.6_amd64.deb
$(LIBSAI_CREDO_BLACKHAWK)_URL = "https://sonicstorage.blob.core.windows.net/public/credosai/libsaicredo-blackhawk_0.9.6_amd64.deb"

ifneq ($($(LIBSAI_CREDO)_URL),)
include $(PLATFORM_PATH)/../template/docker-gbsyncd-base.mk
$(DOCKER_GBSYNCD_BASE)_VERSION = 1.0.0
$(DOCKER_GBSYNCD_BASE)_PACKAGE_NAME = gbsyncd
$(DOCKER_GBSYNCD_BASE)_PATH = $(PLATFORM_PATH)/../components/docker-gbsyncd-$(DOCKER_GBSYNCD_PLATFORM_CODE)
SONIC_ONLINE_DEBS += $(LIBSAI_CREDO) $(LIBSAI_CREDO_OWL) $(LIBSAI_CREDO_BLACKHAWK)
$(DOCKER_GBSYNCD_BASE)_DEPENDS += $(SYNCD) $(LIBSAI_CREDO) $(LIBSAI_CREDO_OWL) $(LIBSAI_CREDO_BLACKHAWK)
endif
