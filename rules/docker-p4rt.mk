# docker image for p4rt

DOCKER_P4RT_STEM = docker-sonic-p4rt
DOCKER_P4RT = $(DOCKER_P4RT_STEM).gz
DOCKER_P4RT_DBG = $(DOCKER_P4RT_STEM)-$(DBG_IMAGE_MARK).gz

$(DOCKER_P4RT)_PATH = $(DOCKERS_PATH)/$(DOCKER_P4RT_STEM)

$(DOCKER_P4RT)_DEPENDS += $(SONIC_P4RT)
$(DOCKER_P4RT)_DBG_DEPENDS = $($(DOCKER_CONFIG_ENGINE_TRIXIE)_DBG_DEPENDS)
$(DOCKER_P4RT)_DBG_DEPENDS += $(SONIC_P4RT_DBG) $(LIBSWSSCOMMON_DBG)
$(DOCKER_P4RT)_DBG_DEPENDS += $(LIBSAIREDIS_DBG)
$(DOCKER_P4RT)_DBG_IMAGE_PACKAGES = $($(DOCKER_CONFIG_ENGINE_TRIXIE)_DBG_IMAGE_PACKAGES)

$(DOCKER_P4RT)_LOAD_DOCKERS += $(DOCKER_CONFIG_ENGINE_TRIXIE)

$(DOCKER_P4RT)_VERSION = 1.0.0
$(DOCKER_P4RT)_PACKAGE_NAME = p4rt
$(DOCKER_P4RT)_WARM_SHUTDOWN_BEFORE = swss
$(DOCKER_P4RT)_FAST_SHUTDOWN_BEFORE = swss

SONIC_DOCKER_IMAGES += $(DOCKER_P4RT)
SONIC_TRIXIE_DOCKERS += $(DOCKER_P4RT)
SONIC_DOCKER_DBG_IMAGES += $(DOCKER_P4RT_DBG)
SONIC_TRIXIE_DBG_DOCKERS += $(DOCKER_P4RT_DBG)

ifeq ($(INCLUDE_P4RT), y)
SONIC_INSTALL_DOCKER_IMAGES += $(DOCKER_P4RT)
SONIC_INSTALL_DOCKER_DBG_IMAGES += $(DOCKER_P4RT_DBG)
endif

$(DOCKER_P4RT)_CONTAINER_NAME = p4rt
$(DOCKER_P4RT)_RUN_OPT += -t
$(DOCKER_P4RT)_RUN_OPT += -v /etc/sonic:/etc/sonic:rw
$(DOCKER_P4RT)_RUN_OPT += -v /etc/localtime:/etc/localtime:ro 
$(DOCKER_P4RT)_RUN_OPT += -v /zmq_swss:/zmq_swss:rw
$(DOCKER_P4RT)_GIT_COMMIT = $(shell cd "$($(SONIC_P4RT)_SRC_PATH)" && git log -n 1 --format=format:"%H %s" || echo "Unable to fetch git log for p4rt")

# Native source OCI images reuse the config-engine parent and @sonic_pins owner TARs.
# Preserve every legacy prerequisite and all excluded profiles above.
ifeq ($(BLDENV),trixie)
SONIC_BAZEL_SWITCHABLE_IMAGES += $(DOCKER_P4RT) $(DOCKER_P4RT_DBG)
ifeq ($(BUILD_WITH_BAZEL_WHEN_AVAILABLE):$(CONFIGURED_PLATFORM):$(CONFIGURED_ARCH):$(INCLUDE_FIPS):$(DBG_IMAGE_MARK),y:vs:amd64:y:dbg)
ifeq ($(filter y,$(CROSS_BUILD_ENVIRON) $(MULTIARCH_QEMU_ENVIRON) $(ENABLE_ASAN)),)
SONIC_BAZEL_DOCKER_IMAGES += $(DOCKER_P4RT)
SONIC_BAZEL_DBG_DOCKER_IMAGES += $(DOCKER_P4RT_DBG)
$(DOCKER_P4RT_DBG)_PATH = $($(DOCKER_P4RT)_PATH)
$(DOCKER_P4RT)_BAZEL_TARGET = //dockers/docker-sonic-p4rt:$(DOCKER_P4RT)
$(DOCKER_P4RT_DBG)_BAZEL_TARGET = //dockers/docker-sonic-p4rt:$(DOCKER_P4RT_DBG)
SONIC_BAZEL_MANIFESTS += docker-sonic-p4rt docker-sonic-p4rt-$(DBG_IMAGE_MARK)
docker-sonic-p4rt_MANIFEST_IMAGE = $(DOCKER_P4RT)
docker-sonic-p4rt-$(DBG_IMAGE_MARK)_MANIFEST_IMAGE = $(DOCKER_P4RT)
docker-sonic-p4rt-$(DBG_IMAGE_MARK)_MANIFEST_SUFFIX = $(DBG_IMAGE_MARK)

$(DOCKER_P4RT)_BAZEL_DEPENDS = $(TARGET_PATH)/bazel-manifests/docker-base-trixie/manifest.json \
    $(TARGET_PATH)/bazel-manifests/docker-config-engine-trixie/manifest.json \
    $(TARGET_PATH)/bazel-manifests/docker-sonic-p4rt/manifest.json
$(DOCKER_P4RT_DBG)_BAZEL_DEPENDS = $($(DOCKER_P4RT)_BAZEL_DEPENDS) \
    $(TARGET_PATH)/bazel-manifests/docker-sonic-p4rt-$(DBG_IMAGE_MARK)/manifest.json
endif
endif
endif
