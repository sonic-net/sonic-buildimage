# docker image for vs gbsyncd

DOCKER_DASH_ENGINE = docker-dash-engine.gz
$(DOCKER_DASH_ENGINE)_VERSION = 1.0.0
$(DOCKER_DASH_ENGINE)_PACKAGE_NAME = dash-engine
$(DOCKER_DASH_ENGINE)_PATH = $(PLATFORM_PATH)/docker-dash-engine

SONIC_DOCKER_IMAGES += $(DOCKER_DASH_ENGINE)
SONIC_INSTALL_DOCKER_IMAGES += $(DOCKER_DASH_ENGINE)


$(DOCKER_DASH_ENGINE)_CONTAINER_NAME = dash_engine
$(DOCKER_DASH_ENGINE)_CONTAINER_PRIVILEGED = true
$(DOCKER_DASH_ENGINE)_RUN_OPT += --privileged -t

# Native source assembly retains this archive's original installer metadata.
# The debug companion is validation-only and is not added to the Make family.
ifeq ($(BLDENV):$(CONFIGURED_PLATFORM):$(CONFIGURED_ARCH),trixie:vs:amd64)
SONIC_BAZEL_SWITCHABLE_IMAGES += $(DOCKER_DASH_ENGINE)
ifeq ($(BUILD_WITH_BAZEL_WHEN_AVAILABLE):$(INCLUDE_FIPS):$(INCLUDE_VS_DASH_SAI),y:y:y)
ifeq ($(filter y,$(CROSS_BUILD_ENVIRON) $(MULTIARCH_QEMU_ENVIRON) $(ENABLE_ASAN) $(ENABLE_SYNCD_RPC)),)
SONIC_BAZEL_DOCKER_IMAGES += $(DOCKER_DASH_ENGINE)
SONIC_BAZEL_MANIFESTS += docker-dash-engine
docker-dash-engine_MANIFEST_IMAGE = $(DOCKER_DASH_ENGINE)
$(DOCKER_DASH_ENGINE)_BAZEL_TARGET = //platform/vs/docker-dash-engine:docker-dash-engine.gz
$(DOCKER_DASH_ENGINE)_BAZEL_DEPENDS = \
    $(TARGET_PATH)/bazel-manifests/docker-base-trixie/manifest.json \
    $(TARGET_PATH)/bazel-manifests/docker-dash-engine/manifest.json
endif
endif
endif

