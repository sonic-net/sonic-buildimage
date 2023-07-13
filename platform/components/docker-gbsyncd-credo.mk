DOCKER_GBSYNCD_PLATFORM_CODE = credo

LIBSAI_CREDO = libsaicredo_0.8.2_amd64.deb
$(LIBSAI_CREDO)_URL = "https://sonicstorage.blob.core.windows.net/packages/credosai/libsaicredo_0.8.2_amd64.deb?sv=2021-04-10&st=2023-01-31T04%3A24%3A23Z&se=2100-01-31T04%3A24%3A00Z&sr=b&sp=r&sig=RZPbmaIetvDRtwifrVT4s%2FaQxB%2FBTOyCqXtMtoNRjmY%3D"
LIBSAI_CREDO_OWL = libsaicredo-owl_0.8.2_amd64.deb
$(LIBSAI_CREDO_OWL)_URL = "https://sonicstorage.blob.core.windows.net/packages/credosai/libsaicredo-owl_0.8.2_amd64.deb?sv=2021-04-10&st=2023-01-31T04%3A25%3A43Z&se=2100-01-31T04%3A25%3A00Z&sr=b&sp=r&sig=%2BdSFujwy0gY%2FiH50Ffi%2FsqZOAHBOFPUcBdR06fHEZkI%3D"

ifneq ($($(LIBSAI_CREDO)_URL),)
include $(PLATFORM_PATH)/../template/docker-gbsyncd-base.mk
$(DOCKER_GBSYNCD_BASE)_VERSION = 1.0.0
$(DOCKER_GBSYNCD_BASE)_PACKAGE_NAME = gbsyncd
$(DOCKER_GBSYNCD_BASE)_PATH = $(PLATFORM_PATH)/../components/docker-gbsyncd-$(DOCKER_GBSYNCD_PLATFORM_CODE)
SONIC_ONLINE_DEBS += $(LIBSAI_CREDO) $(LIBSAI_CREDO_OWL)
$(DOCKER_GBSYNCD_BASE)_DEPENDS += $(SYNCD) $(LIBSAI_CREDO) $(LIBSAI_CREDO_OWL)
endif
