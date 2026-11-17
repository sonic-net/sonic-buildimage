# docker image for lldp agent

DOCKER_LLDP_STEM = docker-lldp
DOCKER_LLDP = $(DOCKER_LLDP_STEM).gz
DOCKER_LLDP_DBG = $(DOCKER_LLDP_STEM)-$(DBG_IMAGE_MARK).gz

$(DOCKER_LLDP)_PATH = $(DOCKERS_PATH)/docker-lldp

$(DOCKER_LLDP)_DEPENDS += $(LLDPD) $(LIBSWSSCOMMON) $(PYTHON3_SWSSCOMMON)

$(DOCKER_LLDP)_DBG_DEPENDS = $($(DOCKER_CONFIG_ENGINE_TRIXIE)_DBG_DEPENDS)
$(DOCKER_LLDP)_DBG_DEPENDS += $(LLDPD_DBG) $(LIBSWSSCOMMON_DBG)

$(DOCKER_LLDP)_DBG_IMAGE_PACKAGES = $($(DOCKER_CONFIG_ENGINE_TRIXIE)_DBG_IMAGE_PACKAGES)

$(DOCKER_LLDP)_PYTHON_WHEELS += $(DBSYNCD_PY3)
$(DOCKER_LLDP)_LOAD_DOCKERS += $(DOCKER_CONFIG_ENGINE_TRIXIE)

$(DOCKER_LLDP)_VERSION = 1.0.0
$(DOCKER_LLDP)_PACKAGE_NAME = lldp
$(DOCKER_LLDP)_WARM_SHUTDOWN_BEFORE = swss
$(DOCKER_LLDP)_FAST_SHUTDOWN_BEFORE = swss

SONIC_DOCKER_IMAGES += $(DOCKER_LLDP)
ifeq ($(INCLUDE_LLDP), y)
SONIC_INSTALL_DOCKER_IMAGES += $(DOCKER_LLDP)
endif

SONIC_DOCKER_DBG_IMAGES += $(DOCKER_LLDP_DBG)
ifeq ($(INCLUDE_LLDP), y)
SONIC_INSTALL_DOCKER_DBG_IMAGES += $(DOCKER_LLDP_DBG)
endif

$(DOCKER_LLDP)_CONTAINER_NAME = lldp
$(DOCKER_LLDP)_RUN_OPT += -t --cap-add=NET_ADMIN
$(DOCKER_LLDP)_RUN_OPT += -v /etc/sonic:/etc/sonic:ro
$(DOCKER_LLDP)_RUN_OPT += -v /etc/localtime:/etc/localtime:ro

$(DOCKER_LLDP)_BASE_IMAGE_FILES += lldpctl:/usr/bin/lldpctl
$(DOCKER_LLDP)_BASE_IMAGE_FILES += lldpcli:/usr/bin/lldpcli

SONIC_TRIXIE_DOCKERS += $(DOCKER_LLDP)
SONIC_TRIXIE_DBG_DOCKERS += $(DOCKER_LLDP_DBG)

# Source-owned LLDP/DBSyncd TARs and the config-engine parent provide runtime files.
# The existing Make producer remains available for other profiles.
ifeq ($(BLDENV),trixie)
SONIC_BAZEL_SWITCHABLE_IMAGES += $(DOCKER_LLDP) $(DOCKER_LLDP_DBG)
ifeq ($(BUILD_WITH_BAZEL_WHEN_AVAILABLE):$(CONFIGURED_PLATFORM):$(CONFIGURED_ARCH):$(INCLUDE_FIPS):$(DBG_IMAGE_MARK),y:vs:amd64:y:dbg)
ifeq ($(filter y,$(CROSS_BUILD_ENVIRON) $(MULTIARCH_QEMU_ENVIRON) $(ENABLE_ASAN)),)
SONIC_BAZEL_DOCKER_IMAGES += $(DOCKER_LLDP)
SONIC_BAZEL_DBG_DOCKER_IMAGES += $(DOCKER_LLDP_DBG)
$(DOCKER_LLDP)_BAZEL_TARGET = //dockers/docker-lldp:$(DOCKER_LLDP)
$(DOCKER_LLDP_DBG)_PATH = $($(DOCKER_LLDP)_PATH)
$(DOCKER_LLDP_DBG)_BAZEL_TARGET = //dockers/docker-lldp:$(DOCKER_LLDP_DBG)
SONIC_BAZEL_MANIFESTS += docker-lldp docker-lldp-dbg
docker-lldp_MANIFEST_IMAGE = $(DOCKER_LLDP)
docker-lldp-dbg_MANIFEST_IMAGE = $(DOCKER_LLDP)
docker-lldp-dbg_MANIFEST_SUFFIX = dbg
$(DOCKER_LLDP)_BAZEL_DEPENDS = $(TARGET_PATH)/bazel-manifests/docker-base-trixie/manifest.json \
    $(TARGET_PATH)/bazel-manifests/docker-config-engine-trixie/manifest.json \
    $(TARGET_PATH)/bazel-manifests/docker-lldp/manifest.json \
    $(TARGET_PATH)/bazel-manifests/docker-lldp/image.env
$(DOCKER_LLDP_DBG)_BAZEL_DEPENDS = $($(DOCKER_LLDP)_BAZEL_DEPENDS) \
    $(TARGET_PATH)/bazel-manifests/docker-lldp-dbg/manifest.json

# Pass the exact Make build setting without interpolating it into shell code.
# Unchanged bytes retain their timestamp; failures retain the previous input.
export SONIC_IMAGE_VERSION
$(TARGET_PATH)/bazel-manifests/docker-lldp/image.env: dockers/docker-lldp/bazel/prepare_version.py bazel-manifests-force
	python3 -B dockers/docker-lldp/bazel/prepare_version.py --output "$@"
endif
endif
endif

$(DOCKER_LLDP)_BAZEL_ARGS = --//dockers/docker-lldp:image_version=$(SONIC_IMAGE_VERSION)
$(DOCKER_LLDP_DBG)_BAZEL_ARGS = $($(DOCKER_LLDP)_BAZEL_ARGS)
