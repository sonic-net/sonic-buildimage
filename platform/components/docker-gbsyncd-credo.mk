DOCKER_GBSYNCD_PLATFORM_CODE = credo

LIBSAI_CREDO = libsaicredo_0.9.0_amd64.deb
$(LIBSAI_CREDO)_URL = "https://sonicstorage.blob.core.windows.net/packages/credosai/libsaicredo_0.9.0_amd64.deb?sv=2021-04-10&st=2023-03-08T02%3A10%3A11Z&se=2100-03-09T02%3A10%3A00Z&sr=b&sp=r&sig=mxOWttgAuOXVjvDI3zF5KHcrTHBgg6mv%2FOpTOxlCoVM%3D"
LIBSAI_CREDO_OWL = libsaicredo-owl_0.9.0_amd64.deb
$(LIBSAI_CREDO_OWL)_URL = "https://sonicstorage.blob.core.windows.net/packages/credosai/libsaicredo-owl_0.9.0_amd64.deb?sv=2021-04-10&st=2023-03-08T02%3A12%3A02Z&se=2100-03-09T02%3A12%3A00Z&sr=b&sp=r&sig=n4mqMVnZxC3u14EWRehfl6bqqUAi1VP1uUdHGg3%2B7H4%3D"

ifneq ($($(LIBSAI_CREDO)_URL),)
include $(PLATFORM_PATH)/../template/docker-gbsyncd-base.mk
$(DOCKER_GBSYNCD_BASE)_VERSION = 1.0.0
$(DOCKER_GBSYNCD_BASE)_PACKAGE_NAME = gbsyncd
$(DOCKER_GBSYNCD_BASE)_PATH = $(PLATFORM_PATH)/../components/docker-gbsyncd-$(DOCKER_GBSYNCD_PLATFORM_CODE)
SONIC_ONLINE_DEBS += $(LIBSAI_CREDO) $(LIBSAI_CREDO_OWL)
$(DOCKER_GBSYNCD_BASE)_DEPENDS += $(SYNCD) $(LIBSAI_CREDO) $(LIBSAI_CREDO_OWL)
endif
