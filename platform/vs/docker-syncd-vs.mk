# docker image for vs syncd

DOCKER_SYNCD_PLATFORM_CODE = vs
include $(PLATFORM_PATH)/../template/docker-syncd-trixie.mk

ifneq ($(BUILD_WITH_BAZEL_WHEN_AVAILABLE),y)
ifneq ($(BUILD_WITH_BAZEL_WHEN_AVAILABLE),n)
$(error BUILD_WITH_BAZEL_WHEN_AVAILABLE must be y or n)
endif
endif

# The global selector applies across platform, architecture and FIPS settings.
# All project packages, including P4/DASH, use source runtime/debug TARs.
# Keep the remaining Trixie, DASH and execution-mode guards below.
ifeq ($(BLDENV),trixie)
SONIC_BAZEL_SWITCHABLE_IMAGES += $(DOCKER_SYNCD_BASE) $(DOCKER_SYNCD_BASE_DBG)
ifeq ($(BUILD_WITH_BAZEL_WHEN_AVAILABLE):$(DBG_IMAGE_MARK):$(INCLUDE_VS_DASH_SAI),y:dbg:y)
ifeq ($(filter y,$(CROSS_BUILD_ENVIRON) $(MULTIARCH_QEMU_ENVIRON) $(ENABLE_ASAN) $(ENABLE_SYNCD_RPC)),)
ifeq ($(findstring docker-sonic-vs,$(SONIC_BUILD_TARGET) $(EXTRA_DOCKER_TARGETS)),)
SONIC_BAZEL_DOCKER_IMAGES += $(DOCKER_SYNCD_BASE)
SONIC_BAZEL_DBG_DOCKER_IMAGES += $(DOCKER_SYNCD_BASE_DBG)

SONIC_BAZEL_MANIFESTS += $(DOCKER_SYNCD_BASE_STEM) $(DOCKER_SYNCD_BASE_STEM)-$(DBG_IMAGE_MARK)
$(DOCKER_SYNCD_BASE_STEM)_MANIFEST_IMAGE = $(DOCKER_SYNCD_BASE)
$(DOCKER_SYNCD_BASE_STEM)-$(DBG_IMAGE_MARK)_MANIFEST_IMAGE = $(DOCKER_SYNCD_BASE)
$(DOCKER_SYNCD_BASE_STEM)-$(DBG_IMAGE_MARK)_MANIFEST_SUFFIX = $(DBG_IMAGE_MARK)

# Source owners build runtime/debug TARs; only service metadata is handed off by Make.
$(DOCKER_SYNCD_BASE)_BAZEL_TARGET = //platform/vs/docker-syncd-vs:docker-syncd-vs.gz
$(DOCKER_SYNCD_BASE_DBG)_BAZEL_TARGET = //platform/vs/docker-syncd-vs:docker-syncd-vs-dbg.gz
$(DOCKER_SYNCD_BASE)_BAZEL_ARGS = --config=syncd-dash
$(DOCKER_SYNCD_BASE_DBG)_BAZEL_ARGS = --config=syncd-dash
$(DOCKER_SYNCD_BASE)_BAZEL_DEPENDS = \
    $(TARGET_PATH)/bazel-manifests/docker-base-trixie/manifest.json \
    $(TARGET_PATH)/bazel-manifests/docker-config-engine-trixie/manifest.json \
    $(TARGET_PATH)/bazel-manifests/docker-syncd-vs/manifest.json
$(DOCKER_SYNCD_BASE_DBG)_BAZEL_DEPENDS = $($(DOCKER_SYNCD_BASE)_BAZEL_DEPENDS) \
    $(TARGET_PATH)/bazel-manifests/docker-syncd-vs-dbg/manifest.json

endif
endif
endif
endif

$(DOCKER_SYNCD_BASE_DBG)_PATH = $($(DOCKER_SYNCD_BASE)_PATH)

$(DOCKER_SYNCD_BASE)_DEPENDS += $(SYNCD_VS) \
                              $(LIBNL3_DEV) \
                              $(LIBNL3)

$(DOCKER_SYNCD_BASE)_DBG_DEPENDS += $(SYNCD_VS_DBG) \
                                $(LIBSWSSCOMMON_DBG) \
                                $(LIBSAIMETADATA_DBG) \
                                $(LIBSAIREDIS_DBG) \
                                $(LIBSAIVS_DBG)

$(DOCKER_SYNCD_BASE)_RUN_OPT += --privileged

$(DOCKER_SYNCD_BASE)_VERSION = 1.0.0
$(DOCKER_SYNCD_BASE)_PACKAGE_NAME = syncd
