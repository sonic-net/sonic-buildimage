DOCKER_GBSYNCD_PLATFORM_CODE = credo

LIBSAI_CREDO = libsaicredo_0.9.3_amd64.deb
$(LIBSAI_CREDO)_URL = "https://sonicstorage.blob.core.windows.net/packages/credosai/libsaicredo_0.9.3_amd64.deb?sv=2021-04-10&st=2023-10-12T02%3A21%3A05Z&se=2031-10-13T02%3A21%3A00Z&sr=b&sp=r&sig=UXC%2FYKm%2BvHRjGmM3xjnFMQzY%2BMpxhKtMxNHQPdwvtN8%3D"
LIBSAI_CREDO_OWL = libsaicredo-owl_0.9.3_amd64.deb
$(LIBSAI_CREDO_OWL)_URL = "https://sonicstorage.blob.core.windows.net/packages/credosai/libsaicredo-owl_0.9.3_amd64.deb?sv=2021-04-10&st=2023-10-12T02%3A21%3A51Z&se=2031-10-13T02%3A21%3A00Z&sr=b&sp=r&sig=olu3%2Bq5eJYRtXCygJWgKUx%2FdlrlB%2FWE0i9ruftYdB7g%3D"

ifneq ($($(LIBSAI_CREDO)_URL),)
include $(PLATFORM_PATH)/../template/docker-gbsyncd-base.mk
$(DOCKER_GBSYNCD_BASE)_VERSION = 1.0.0
$(DOCKER_GBSYNCD_BASE)_PACKAGE_NAME = gbsyncd
$(DOCKER_GBSYNCD_BASE)_PATH = $(PLATFORM_PATH)/../components/docker-gbsyncd-$(DOCKER_GBSYNCD_PLATFORM_CODE)
SONIC_ONLINE_DEBS += $(LIBSAI_CREDO) $(LIBSAI_CREDO_OWL)
$(DOCKER_GBSYNCD_BASE)_DEPENDS += $(SYNCD) $(LIBSAI_CREDO) $(LIBSAI_CREDO_OWL)
endif
