# SONiC SDK Docker Image

DOCKER_SONIC_SDK_BUILDENV_STEM = sonic-sdk-buildenv
DOCKER_SONIC_SDK_BUILDENV = $(DOCKER_SONIC_SDK_BUILDENV_STEM).gz
DOCKER_SONIC_SDK_BUILDENV_DBG = $(DOCKER_SONIC_SDK_BUILDENV_STEM)-$(DBG_IMAGE_MARK).gz

$(DOCKER_SONIC_SDK_BUILDENV)_PATH = $(DOCKERS_PATH)/docker-sonic-sdk-buildenv

$(DOCKER_SONIC_SDK_BUILDENV)_DEPENDS += $(LIBSAIVS) \
                                        $(LIBSAIVS_DEV) \
                                        $(LIBSAIREDIS_DEV) \
                                        $(LIBSAIMETADATA_DEV) \
                                        $(LIBSWSSCOMMON_DEV) \
                                        $(LIBNL3_DEV) \
                                        $(LIBNL_GENL3_DEV) \
                                        $(LIBNL_ROUTE3_DEV) \
                                        $(LIBNL_NF3_DEV) \
                                        $(LIBNL_CLI_DEV)

$(DOCKER_SONIC_SDK_BUILDENV)_DBG_DEPENDS = $($(DOCKER_SONIC_SDK)_DBG_DEPENDS) \
                                           $(LIBSAIVS_DBG)
$(DOCKER_SONIC_SDK_BUILDENV)_DBG_IMAGE_PACKAGES = $($(DOCKER_SONIC_SDK)_DBG_IMAGE_PACKAGES)


$(DOCKER_SONIC_SDK_BUILDENV)_LOAD_DOCKERS += $(DOCKER_SONIC_SDK)

SONIC_DOCKER_IMAGES += $(DOCKER_SONIC_SDK_BUILDENV)
SONIC_DOCKER_DBG_IMAGES += $(DOCKER_SONIC_SDK_BUILDENV_DBG)

SONIC_BULLSEYE_DOCKERS += $(DOCKER_SONIC_SDK_BUILDENV)
SONIC_BULLSEYE_DBG_DOCKERS += $(DOCKER_SONIC_SDK_BUILDENV_DBG)
