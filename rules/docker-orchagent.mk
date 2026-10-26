# docker image for orchagent

DOCKER_ORCHAGENT_STEM = docker-orchagent
DOCKER_ORCHAGENT = $(DOCKER_ORCHAGENT_STEM).gz
DOCKER_ORCHAGENT_DBG = $(DOCKER_ORCHAGENT_STEM)-$(DBG_IMAGE_MARK).gz

ifneq ($(BUILD_WITH_BAZEL_WHEN_AVAILABLE),y)
ifneq ($(BUILD_WITH_BAZEL_WHEN_AVAILABLE),n)
$(error BUILD_WITH_BAZEL_WHEN_AVAILABLE must be y or n)
endif
endif

# The top-level Make build still visits bookworm for its legacy dependencies.
# Select Bazel only in the trixie slave that owns the installed SWSS container.
ifeq ($(BLDENV),trixie)
SONIC_BAZEL_SWITCHABLE_IMAGES += $(DOCKER_ORCHAGENT) $(DOCKER_ORCHAGENT_DBG)
# Register only the supported Bazel configuration. The global selector leaves
# every other configuration on Make, including native ARM64 and ASAN builds.
ifeq ($(BUILD_WITH_BAZEL_WHEN_AVAILABLE):$(CONFIGURED_PLATFORM):$(CONFIGURED_ARCH),y:vs:amd64)
ifeq ($(filter y,$(CROSS_BUILD_ENVIRON) $(MULTIARCH_QEMU_ENVIRON) $(ENABLE_ASAN)),)
SONIC_BAZEL_DOCKER_IMAGES += $(DOCKER_ORCHAGENT)
SONIC_BAZEL_DBG_DOCKER_IMAGES += $(DOCKER_ORCHAGENT_DBG)
ifeq ($(INCLUDE_FIPS),y)
# Match the config-engine source selector. Its Docker archive intentionally has
# no native OCI layout; Bazel consumes the existing source image graph instead.
$(DOCKER_ORCHAGENT)_BAZEL_ARGS = --//dockers/docker-orchagent:source_config_engine=true
$(DOCKER_ORCHAGENT_DBG)_BAZEL_ARGS = $($(DOCKER_ORCHAGENT)_BAZEL_ARGS)
$(DOCKER_ORCHAGENT)_BAZEL_DEPENDS = $(TARGET_PATH)/bazel-manifests/docker-base-trixie/manifest.json \
    $(TARGET_PATH)/bazel-manifests/docker-config-engine-trixie/manifest.json $(PYTHON_WHEELS_PATH)/$(SCAPY)
else
SONIC_BAZEL_OCI_BASES += docker-config-engine-trixie.oci
docker-config-engine-trixie.oci_OCI_ARCHIVE = $(TARGET_PATH)/$(DOCKER_CONFIG_ENGINE_TRIXIE)
docker-config-engine-trixie.oci_OCI_PLATFORM = linux/amd64
$(DOCKER_ORCHAGENT)_BAZEL_DEPENDS = $(TARGET_PATH)/docker-config-engine-trixie.oci $(PYTHON_WHEELS_PATH)/$(SCAPY)
endif
$(DOCKER_ORCHAGENT)_BAZEL_TARGET = //dockers/docker-orchagent:$(DOCKER_ORCHAGENT)
$(DOCKER_ORCHAGENT_DBG)_BAZEL_TARGET = //dockers/docker-orchagent:$(DOCKER_ORCHAGENT_DBG)
# Both variants use runtime metadata, with Make adding the debug suffix.
SONIC_BAZEL_MANIFESTS += $(DOCKER_ORCHAGENT_STEM) $(DOCKER_ORCHAGENT_STEM)-$(DBG_IMAGE_MARK)
$(DOCKER_ORCHAGENT_STEM)_MANIFEST_IMAGE = $(DOCKER_ORCHAGENT)
$(DOCKER_ORCHAGENT_STEM)-$(DBG_IMAGE_MARK)_MANIFEST_IMAGE = $(DOCKER_ORCHAGENT)
$(DOCKER_ORCHAGENT_STEM)-$(DBG_IMAGE_MARK)_MANIFEST_SUFFIX = $(DBG_IMAGE_MARK)
$(DOCKER_ORCHAGENT)_BAZEL_DEPENDS += $(TARGET_PATH)/bazel-manifests/$(DOCKER_ORCHAGENT_STEM)/manifest.json
# The debug OCI image extends the runtime image, so it needs both manifests.
$(DOCKER_ORCHAGENT_DBG)_BAZEL_DEPENDS = $($(DOCKER_ORCHAGENT)_BAZEL_DEPENDS) $(TARGET_PATH)/bazel-manifests/$(DOCKER_ORCHAGENT_STEM)-$(DBG_IMAGE_MARK)/manifest.json
endif
endif
endif

# Keep the legacy metadata for consumers such as docker-sonic-vs, which merges
# these package and wheel lists into its own Make build. The Bazel SWSS recipe
# below uses its own explicit prerequisites instead of these package targets.
$(DOCKER_ORCHAGENT)_DEPENDS += $(SWSS) $(LIB_SONIC_DASH_API)

ifeq ($(ENABLE_ASAN), y)
$(DOCKER_ORCHAGENT)_DEPENDS += $(SWSS_DBG)
endif

$(DOCKER_ORCHAGENT)_DBG_DEPENDS = $($(DOCKER_SWSS_LAYER_TRIXIE)_DBG_DEPENDS)
$(DOCKER_ORCHAGENT)_DBG_DEPENDS +=   $(SWSS_DBG) \
                                $(LIBSWSSCOMMON_DBG) \
                                $(LIBSAIREDIS_DBG)
$(DOCKER_ORCHAGENT)_PYTHON_WHEELS += $(SCAPY)

$(DOCKER_ORCHAGENT)_DBG_IMAGE_PACKAGES = $($(DOCKER_SWSS_LAYER_TRIXIE)_DBG_IMAGE_PACKAGES)

$(DOCKER_ORCHAGENT)_PATH = $(DOCKERS_PATH)/$(DOCKER_ORCHAGENT_STEM)
$(DOCKER_ORCHAGENT_DBG)_PATH = $($(DOCKER_ORCHAGENT)_PATH)

$(DOCKER_ORCHAGENT)_LOAD_DOCKERS += $(DOCKER_SWSS_LAYER_TRIXIE)

$(DOCKER_ORCHAGENT)_VERSION = 1.0.0
$(DOCKER_ORCHAGENT)_PACKAGE_NAME = swss
$(DOCKER_ORCHAGENT)_WARM_SHUTDOWN_BEFORE = syncd
$(DOCKER_ORCHAGENT)_FAST_SHUTDOWN_BEFORE = syncd

SONIC_DOCKER_IMAGES += $(DOCKER_ORCHAGENT)
SONIC_TRIXIE_DOCKERS += $(DOCKER_ORCHAGENT)
SONIC_INSTALL_DOCKER_IMAGES += $(DOCKER_ORCHAGENT)

SONIC_DOCKER_DBG_IMAGES += $(DOCKER_ORCHAGENT_DBG)
SONIC_TRIXIE_DBG_DOCKERS += $(DOCKER_ORCHAGENT_DBG)
SONIC_INSTALL_DOCKER_DBG_IMAGES += $(DOCKER_ORCHAGENT_DBG)

$(DOCKER_ORCHAGENT)_CONTAINER_NAME = swss
$(DOCKER_ORCHAGENT)_RUN_OPT += -t --cap-add=NET_ADMIN --security-opt apparmor=unconfined --security-opt="systempaths=unconfined"
ifeq ($(ENABLE_ASAN), y)
$(DOCKER_ORCHAGENT)_RUN_OPT += --cap-add=SYS_PTRACE
endif
$(DOCKER_ORCHAGENT)_RUN_OPT += -v /etc/network/interfaces:/etc/network/interfaces:ro
$(DOCKER_ORCHAGENT)_RUN_OPT += -v /etc/localtime:/etc/localtime:ro 
$(DOCKER_ORCHAGENT)_RUN_OPT += -v /etc/network/interfaces.d/:/etc/network/interfaces.d/:ro
$(DOCKER_ORCHAGENT)_RUN_OPT += -v /host/machine.conf:/host/machine.conf:ro
$(DOCKER_ORCHAGENT)_RUN_OPT += -v /etc/sonic:/etc/sonic:ro
$(DOCKER_ORCHAGENT)_RUN_OPT += -v /var/log/swss:/var/log/swss:rw
$(DOCKER_ORCHAGENT)_RUN_OPT += -v /zmq_swss:/zmq_swss:rw

$(DOCKER_ORCHAGENT)_BASE_IMAGE_FILES += swssloglevel:/usr/bin/swssloglevel
$(DOCKER_ORCHAGENT)_FILES += $(ARP_UPDATE_SCRIPT) $(ARP_UPDATE_VARS_TEMPLATE)
