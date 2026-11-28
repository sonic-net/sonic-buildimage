# trixie-based docker image for sonic swss layer

DOCKER_SWSS_LAYER_TRIXIE = docker-swss-layer-trixie.gz
$(DOCKER_SWSS_LAYER_TRIXIE)_PATH = $(DOCKERS_PATH)/docker-swss-layer-trixie

$(DOCKER_SWSS_LAYER_TRIXIE)_DEPENDS += $(SWSS)
$(DOCKER_SWSS_LAYER_TRIXIE)_LOAD_DOCKERS += $(DOCKER_CONFIG_ENGINE_TRIXIE)

$(DOCKER_SWSS_LAYER_TRIXIE)_DBG_DEPENDS = $($(DOCKER_CONFIG_ENGINE_TRIXIE)_DBG_DEPENDS) \
                                             $(SWSS_DBG)
$(DOCKER_SWSS_LAYER_TRIXIE)_DBG_IMAGE_PACKAGES = $($(DOCKER_CONFIG_ENGINE_TRIXIE)_DBG_IMAGE_PACKAGES)

SONIC_DOCKER_IMAGES += $(DOCKER_SWSS_LAYER_TRIXIE)
SONIC_TRIXIE_DOCKERS += $(DOCKER_SWSS_LAYER_TRIXIE)

# Keep legacy declarations for unconverted consumers while selecting the shared
# source-built archive through the existing Make/Bazel handoff on native AMD64.
ifeq ($(BLDENV),trixie)
SONIC_BAZEL_MANIFESTS += docker-swss-layer-trixie
docker-swss-layer-trixie_MANIFEST_IMAGE = $(DOCKER_SWSS_LAYER_TRIXIE)
ifeq ($(CONFIGURED_ARCH),amd64)
ifeq ($(CONFIGURED_PLATFORM),vs)
SONIC_BAZEL_SWITCHABLE_IMAGES += $(DOCKER_SWSS_LAYER_TRIXIE)
ifeq ($(BUILD_WITH_BAZEL_WHEN_AVAILABLE),y)
ifeq ($(filter y,$(CROSS_BUILD_ENVIRON) $(MULTIARCH_QEMU_ENVIRON) $(ENABLE_ASAN)),)
SONIC_BAZEL_DOCKER_IMAGES += $(DOCKER_SWSS_LAYER_TRIXIE)
$(DOCKER_SWSS_LAYER_TRIXIE)_BAZEL_TARGET = //dockers/docker-swss-layer-trixie:docker-swss-layer-trixie.gz
$(DOCKER_SWSS_LAYER_TRIXIE)_BAZEL_DEPENDS = $(TARGET_PATH)/bazel-manifests/docker-swss-layer-trixie/manifest.json
$(DOCKER_SWSS_LAYER_TRIXIE)_BAZEL_DEPENDS += $(TARGET_PATH)/bazel-manifests/docker-config-engine-trixie/manifest.json
$(DOCKER_SWSS_LAYER_TRIXIE)_BAZEL_DEPENDS += $(TARGET_PATH)/bazel-manifests/docker-base-trixie/manifest.json
endif
endif
endif
endif
endif
