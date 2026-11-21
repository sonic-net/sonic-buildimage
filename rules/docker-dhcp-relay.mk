# Docker image for DHCP relay

DOCKER_DHCP_RELAY_STEM = docker-dhcp-relay
DOCKER_DHCP_RELAY = $(DOCKER_DHCP_RELAY_STEM).gz
DOCKER_DHCP_RELAY_DBG = $(DOCKER_DHCP_RELAY_STEM)-$(DBG_IMAGE_MARK).gz

$(DOCKER_DHCP_RELAY)_PATH = $(DOCKERS_PATH)/$(DOCKER_DHCP_RELAY_STEM)

$(DOCKER_DHCP_RELAY)_DEPENDS += $(ISC_DHCP_RELAY) $(SONIC_DHCPMON) $(SONIC_DHCP4RELAY) $(SONIC_DHCPRELAY) $(LIBSWSSCOMMON)

$(DOCKER_DHCP_RELAY)_DBG_DEPENDS = $($(DOCKER_CONFIG_ENGINE_TRIXIE)_DBG_DEPENDS)
$(DOCKER_DHCP_RELAY)_DBG_DEPENDS += $(ISC_DHCP_RELAY_DBG) $(SONIC_DHCP4RELAY_DBG) $(SONIC_DHCPRELAY_DBG) $(SONIC_DHCPMON_DBG)

$(DOCKER_DHCP_RELAY)_DBG_IMAGE_PACKAGES = $($(DOCKER_CONFIG_ENGINE_TRIXIE)_DBG_IMAGE_PACKAGES)

$(DOCKER_DHCP_RELAY)_LOAD_DOCKERS = $(DOCKER_CONFIG_ENGINE_TRIXIE)

$(DOCKER_DHCP_RELAY)_INSTALL_PYTHON_WHEELS = $(SONIC_UTILITIES_PY3)
$(DOCKER_DHCP_RELAY)_INSTALL_DEBS = $(PYTHON3_SWSSCOMMON)
$(DOCKER_DHCP_RELAY)_PYTHON_WHEELS += $(SONIC_DHCP_UTILITIES_PY3)

$(DOCKER_DHCP_RELAY)_VERSION = 1.0.0
$(DOCKER_DHCP_RELAY)_PACKAGE_NAME = dhcp-relay
$(DOCKER_DHCP_RELAY)_PACKAGE_DEPENDS = database^1.0.0

$(DOCKER_DHCP_RELAY)_SERVICE_REQUIRES = config-setup
$(DOCKER_DHCP_RELAY)_SERVICE_AFTER = swss syncd teamd
$(DOCKER_DHCP_RELAY)_SERVICE_DEPENDENT_OF = swss

SONIC_DOCKER_IMAGES += $(DOCKER_DHCP_RELAY)
SONIC_TRIXIE_DOCKERS += $(DOCKER_DHCP_RELAY)
SONIC_DOCKER_DBG_IMAGES += $(DOCKER_DHCP_RELAY_DBG)
SONIC_TRIXIE_DBG_DOCKERS += $(DOCKER_DHCP_RELAY_DBG)

ifeq ($(INCLUDE_KUBERNETES),y)
$(DOCKER_DHCP_RELAY)_DEFAULT_FEATURE_OWNER = kube
endif

$(DOCKER_DHCP_RELAY)_DEFAULT_FEATURE_STATE_ENABLED = y

ifeq ($(INCLUDE_DHCP_RELAY),y)
ifeq ($(INSTALL_DEBUG_TOOLS),y)
SONIC_PACKAGES_LOCAL += $(DOCKER_DHCP_RELAY_DBG)
else
SONIC_PACKAGES_LOCAL += $(DOCKER_DHCP_RELAY)
endif
endif

$(DOCKER_DHCP_RELAY)_CONTAINER_NAME = dhcp_relay
$(DOCKER_DHCP_RELAY)_CONTAINER_PRIVILEGED = false
$(DOCKER_DHCP_RELAY)_CONTAINER_VOLUMES += /etc/sonic:/etc/sonic:ro
$(DOCKER_DHCP_RELAY)_CONTAINER_VOLUMES += /etc/localtime:/etc/localtime:ro
$(DOCKER_DHCP_RELAY)_CONTAINER_TMPFS += /tmp/
$(DOCKER_DHCP_RELAY)_CONTAINER_TMPFS += /var/tmp/

$(DOCKER_DHCP_RELAY)_CLI_CONFIG_PLUGIN = /cli/config/plugins/dhcp_relay.py
$(DOCKER_DHCP_RELAY)_CLI_SHOW_PLUGIN = /cli/show/plugins/show_dhcp_relay.py
$(DOCKER_DHCP_RELAY)_CLI_CLEAR_PLUGIN = /cli/clear/plugins/clear_dhcp_relay.py

# Native source TARs, a source-owned utility wheel and the config-engine parent
# provide the supported VS image. Other profiles retain their Make producers.
ifeq ($(BLDENV),trixie)
SONIC_BAZEL_SWITCHABLE_IMAGES += $(DOCKER_DHCP_RELAY) $(DOCKER_DHCP_RELAY_DBG)
ifeq ($(BUILD_WITH_BAZEL_WHEN_AVAILABLE):$(CONFIGURED_PLATFORM):$(CONFIGURED_ARCH):$(INCLUDE_FIPS):$(DBG_IMAGE_MARK),y:vs:amd64:y:dbg)
ifeq ($(filter y,$(CROSS_BUILD_ENVIRON) $(MULTIARCH_QEMU_ENVIRON) $(ENABLE_ASAN)),)
SONIC_BAZEL_DOCKER_IMAGES += $(DOCKER_DHCP_RELAY)
SONIC_BAZEL_DBG_DOCKER_IMAGES += $(DOCKER_DHCP_RELAY_DBG)
$(DOCKER_DHCP_RELAY)_BAZEL_TARGET = //dockers/docker-dhcp-relay:$(DOCKER_DHCP_RELAY)
$(DOCKER_DHCP_RELAY_DBG)_PATH = $($(DOCKER_DHCP_RELAY)_PATH)
$(DOCKER_DHCP_RELAY_DBG)_BAZEL_TARGET = //dockers/docker-dhcp-relay:$(DOCKER_DHCP_RELAY_DBG)
SONIC_BAZEL_MANIFESTS += docker-dhcp-relay docker-dhcp-relay-dbg
docker-dhcp-relay_MANIFEST_IMAGE = $(DOCKER_DHCP_RELAY)
docker-dhcp-relay-dbg_MANIFEST_IMAGE = $(DOCKER_DHCP_RELAY)
docker-dhcp-relay-dbg_MANIFEST_SUFFIX = dbg
$(DOCKER_DHCP_RELAY)_BAZEL_DEPENDS = $(TARGET_PATH)/bazel-manifests/docker-base-trixie/manifest.json \
    $(TARGET_PATH)/bazel-manifests/docker-config-engine-trixie/manifest.json \
    $(TARGET_PATH)/bazel-manifests/docker-dhcp-relay/manifest.json \
    $(TARGET_PATH)/bazel-manifests/docker-dhcp-relay/image.env
$(DOCKER_DHCP_RELAY_DBG)_BAZEL_DEPENDS = $($(DOCKER_DHCP_RELAY)_BAZEL_DEPENDS) \
    $(TARGET_PATH)/bazel-manifests/docker-dhcp-relay-dbg/manifest.json

# Pass the exact Make setting as data. Unchanged values keep their timestamps,
# and an invalid value cannot replace a previously prepared input.
export SONIC_IMAGE_VERSION
$(TARGET_PATH)/bazel-manifests/docker-dhcp-relay/image.env: tools/bazel/oci/prepare_image_version.py bazel-manifests-force
	python3 -B tools/bazel/oci/prepare_image_version.py --output "$@"
endif
endif
endif
