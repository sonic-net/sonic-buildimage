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

# The source owner intentionally retains Bazel 6.4.0. Prepare its declared
# Common SDK and TAR repository before the normal Bazel 8.5.1 OCI build.
# Both clients reuse the slave's Bazel caches; owner server state is separate.
# These targets do not run tests or create Debian packages.
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

P4RT_BAZEL ?= bazel
P4RT_BAZEL_JOBS ?= 1
P4RT_BAZEL_OUTPUT_BASE ?= $(HOME)/.cache/sonic-p4rt-owner/$(notdir $(CURDIR))
P4RT_BAZEL_ARTIFACTS ?= artifacts/p4rt
P4RT_BAZEL_SOURCE = src/sonic-p4rt/sonic-pins
# Resolve the committed gitlink only when preparation is requested. Manifest
# generation reads metadata without building or initializing the source owner.
P4RT_BAZEL_REVISION = $(shell git rev-parse HEAD:$(P4RT_BAZEL_SOURCE))
P4RT_BAZEL_PARENT_MANIFESTS = $(TARGET_PATH)/bazel-manifests/docker-base-trixie/manifest.json \
    $(TARGET_PATH)/bazel-manifests/docker-config-engine-trixie/manifest.json

.PHONY: bazel-p4rt-owner
bazel-p4rt-owner: $(P4RT_BAZEL_PARENT_MANIFESTS)
	python3 -B tools/bazel/ci/p4rt_image.py prepare-make-owner \
		--directory "$(P4RT_BAZEL_ARTIFACTS)" --bazel "$${BAZEL:-bazel}" \
		--owner-source "$(P4RT_BAZEL_SOURCE)" --owner-revision "$(P4RT_BAZEL_REVISION)" \
		--owner-bazel "$(P4RT_BAZEL)" --owner-output-base "$(P4RT_BAZEL_OUTPUT_BASE)" \
		--jobs "$(P4RT_BAZEL_JOBS)"

$(DOCKER_P4RT)_BAZEL_DEPENDS = $(P4RT_BAZEL_PARENT_MANIFESTS) \
    $(TARGET_PATH)/bazel-manifests/docker-sonic-p4rt/manifest.json bazel-p4rt-owner
$(DOCKER_P4RT_DBG)_BAZEL_DEPENDS = $($(DOCKER_P4RT)_BAZEL_DEPENDS) \
    $(TARGET_PATH)/bazel-manifests/docker-sonic-p4rt-$(DBG_IMAGE_MARK)/manifest.json
endif
endif
endif
