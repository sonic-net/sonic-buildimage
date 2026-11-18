# docker image for otel collector

DOCKER_OTEL_STEM = docker-sonic-otel
DOCKER_OTEL = $(DOCKER_OTEL_STEM).gz
DOCKER_OTEL_DBG = $(DOCKER_OTEL_STEM)-$(DBG_IMAGE_MARK).gz

$(DOCKER_OTEL)_PATH = $(DOCKERS_PATH)/$(DOCKER_OTEL_STEM)

$(DOCKER_OTEL)_DBG_DEPENDS = $($(DOCKER_CONFIG_ENGINE_TRIXIE)_DBG_DEPENDS)
$(DOCKER_OTEL)_LOAD_DOCKERS += $(DOCKER_CONFIG_ENGINE_TRIXIE)
$(DOCKER_OTEL)_VERSION = 1.0.0
$(DOCKER_OTEL)_PACKAGE_NAME = otel
$(DOCKER_OTEL)_DBG_IMAGE_PACKAGES = $($(DOCKER_CONFIG_ENGINE_TRIXIE)_DBG_IMAGE_PACKAGES)

SONIC_DOCKER_IMAGES += $(DOCKER_OTEL)
SONIC_TRIXIE_DOCKERS += $(DOCKER_OTEL)
ifeq ($(INCLUDE_SYSTEM_OTEL), y)
SONIC_INSTALL_DOCKER_IMAGES += $(DOCKER_OTEL)
endif

SONIC_DOCKER_DBG_IMAGES += $(DOCKER_OTEL_DBG)
SONIC_TRIXIE_DBG_DOCKERS += $(DOCKER_OTEL_DBG)
ifeq ($(INCLUDE_SYSTEM_OTEL), y)
SONIC_INSTALL_DOCKER_DBG_IMAGES += $(DOCKER_OTEL_DBG)
endif

$(DOCKER_OTEL)_CONTAINER_NAME = otel
$(DOCKER_OTEL)_RUN_OPT += -t
$(DOCKER_OTEL)_RUN_OPT += -v /etc/sonic:/etc/sonic:ro
$(DOCKER_OTEL)_RUN_OPT += -v /etc/localtime:/etc/localtime:ro
$(DOCKER_OTEL)_RUN_OPT += -v /:/mnt/host:ro
$(DOCKER_OTEL)_RUN_OPT += -v /tmp:/mnt/host/tmp:rw
$(DOCKER_OTEL)_RUN_OPT += -v /var/tmp:/mnt/host/var/tmp:rw
$(DOCKER_OTEL)_RUN_OPT += --pid=host
$(DOCKER_OTEL)_RUN_OPT += --userns=host

$(DOCKER_OTEL)_BASE_IMAGE_FILES += monit_otel:/etc/monit/conf.d


# Native source OCI images reuse the config-engine parent and the collector owner TARs.
# Preserve every legacy prerequisite and all excluded profiles above.
ifeq ($(BLDENV),trixie)
SONIC_BAZEL_SWITCHABLE_IMAGES += $(DOCKER_OTEL) $(DOCKER_OTEL_DBG)
ifeq ($(BUILD_WITH_BAZEL_WHEN_AVAILABLE):$(CONFIGURED_PLATFORM):$(CONFIGURED_ARCH):$(INCLUDE_FIPS):$(DBG_IMAGE_MARK),y:vs:amd64:y:dbg)
ifeq ($(filter y,$(CROSS_BUILD_ENVIRON) $(MULTIARCH_QEMU_ENVIRON) $(ENABLE_ASAN)),)
SONIC_BAZEL_DOCKER_IMAGES += $(DOCKER_OTEL)
SONIC_BAZEL_DBG_DOCKER_IMAGES += $(DOCKER_OTEL_DBG)
$(DOCKER_OTEL_DBG)_PATH = $($(DOCKER_OTEL)_PATH)
$(DOCKER_OTEL)_BAZEL_TARGET = //dockers/docker-sonic-otel:$(DOCKER_OTEL)
$(DOCKER_OTEL_DBG)_BAZEL_TARGET = //dockers/docker-sonic-otel:$(DOCKER_OTEL_DBG)
SONIC_BAZEL_IMAGE_ENVIRONMENTS += docker-sonic-otel
SONIC_BAZEL_MANIFESTS += docker-sonic-otel docker-sonic-otel-$(DBG_IMAGE_MARK)
docker-sonic-otel_MANIFEST_IMAGE = $(DOCKER_OTEL)
docker-sonic-otel-$(DBG_IMAGE_MARK)_MANIFEST_IMAGE = $(DOCKER_OTEL)
docker-sonic-otel-$(DBG_IMAGE_MARK)_MANIFEST_SUFFIX = $(DBG_IMAGE_MARK)
$(DOCKER_OTEL)_BAZEL_DEPENDS = $(TARGET_PATH)/bazel-manifests/docker-sonic-otel/image.env \
    $(TARGET_PATH)/bazel-manifests/docker-base-trixie/manifest.json \
    $(TARGET_PATH)/bazel-manifests/docker-config-engine-trixie/manifest.json \
    $(TARGET_PATH)/bazel-manifests/docker-sonic-otel/manifest.json
$(DOCKER_OTEL_DBG)_BAZEL_DEPENDS = $($(DOCKER_OTEL)_BAZEL_DEPENDS) $(TARGET_PATH)/bazel-manifests/docker-sonic-otel-$(DBG_IMAGE_MARK)/manifest.json
endif
endif
endif
