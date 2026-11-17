# docker image for vs gbsyncd

DOCKER_GBSYNCD_PLATFORM_CODE = vs
include $(PLATFORM_PATH)/../template/docker-gbsyncd-trixie.mk

$(DOCKER_GBSYNCD_BASE)_DEPENDS += $(SYNCD_VS) \
                              $(LIBNL3_DEV) \
                              $(LIBNL3)

$(DOCKER_GBSYNCD_BASE)_DBG_DEPENDS += $(SYNCD_VS_DBG) \
                                $(LIBSWSSCOMMON_DBG) \
                                $(LIBSAIMETADATA_DBG) \
                                $(LIBSAIREDIS_DBG) \
                                $(LIBSAIVS_DBG)

$(DOCKER_GBSYNCD_BASE)_VERSION = 1.0.0
$(DOCKER_GBSYNCD_BASE)_PACKAGE_NAME = gbsyncd


# Reuse the source-built VS/DASH package closure; Make hands off metadata only.
# Other profiles keep the existing producer, without changing its prerequisites.
ifeq ($(BLDENV),trixie)
SONIC_BAZEL_SWITCHABLE_IMAGES += $(DOCKER_GBSYNCD_BASE) $(DOCKER_GBSYNCD_BASE_DBG)
ifeq ($(BUILD_WITH_BAZEL_WHEN_AVAILABLE):$(CONFIGURED_PLATFORM):$(CONFIGURED_ARCH):$(INCLUDE_FIPS):$(INCLUDE_VS_DASH_SAI):$(DBG_IMAGE_MARK),y:vs:amd64:y:y:dbg)
ifeq ($(filter y,$(CROSS_BUILD_ENVIRON) $(MULTIARCH_QEMU_ENVIRON) $(ENABLE_ASAN) $(ENABLE_SYNCD_RPC)),)
ifeq ($(findstring docker-sonic-vs,$(SONIC_BUILD_TARGET) $(EXTRA_DOCKER_TARGETS)),)
SONIC_BAZEL_DOCKER_IMAGES += $(DOCKER_GBSYNCD_BASE)
SONIC_BAZEL_DBG_DOCKER_IMAGES += $(DOCKER_GBSYNCD_BASE_DBG)
$(DOCKER_GBSYNCD_BASE)_BAZEL_TARGET = //platform/vs/docker-gbsyncd-vs:$(DOCKER_GBSYNCD_BASE)
$(DOCKER_GBSYNCD_BASE_DBG)_BAZEL_TARGET = //platform/vs/docker-gbsyncd-vs:$(DOCKER_GBSYNCD_BASE_DBG)
$(DOCKER_GBSYNCD_BASE)_BAZEL_ARGS = --config=syncd-dash
$(DOCKER_GBSYNCD_BASE_DBG)_BAZEL_ARGS = --config=syncd-dash
SONIC_BAZEL_IMAGE_ENVIRONMENTS += docker-gbsyncd-vs
SONIC_BAZEL_MANIFESTS += docker-gbsyncd-vs docker-gbsyncd-vs-dbg
docker-gbsyncd-vs_MANIFEST_IMAGE = $(DOCKER_GBSYNCD_BASE)
docker-gbsyncd-vs-dbg_MANIFEST_IMAGE = $(DOCKER_GBSYNCD_BASE)
docker-gbsyncd-vs-dbg_MANIFEST_SUFFIX = dbg
$(DOCKER_GBSYNCD_BASE)_BAZEL_DEPENDS = $(TARGET_PATH)/bazel-manifests/docker-base-trixie/manifest.json \
    $(TARGET_PATH)/bazel-manifests/docker-config-engine-trixie/manifest.json \
    $(TARGET_PATH)/bazel-manifests/docker-gbsyncd-vs/manifest.json \
    $(TARGET_PATH)/bazel-manifests/docker-gbsyncd-vs/image.env
$(DOCKER_GBSYNCD_BASE_DBG)_BAZEL_DEPENDS = $($(DOCKER_GBSYNCD_BASE)_BAZEL_DEPENDS) \
    $(TARGET_PATH)/bazel-manifests/docker-gbsyncd-vs-dbg/manifest.json
endif
endif
endif
endif
