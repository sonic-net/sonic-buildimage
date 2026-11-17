# docker image for GNMI agent

DOCKER_GNMI_STEM = docker-sonic-gnmi
DOCKER_GNMI = $(DOCKER_GNMI_STEM).gz
DOCKER_GNMI_DBG = $(DOCKER_GNMI_STEM)-$(DBG_IMAGE_MARK).gz

$(DOCKER_GNMI)_PATH = $(DOCKERS_PATH)/$(DOCKER_GNMI_STEM)

$(DOCKER_GNMI)_DEPENDS += $(SONIC_MGMT_COMMON)
$(DOCKER_GNMI)_DEPENDS += $(SONIC_TELEMETRY)
$(DOCKER_GNMI)_DBG_DEPENDS = $($(DOCKER_CONFIG_ENGINE_TRIXIE)_DBG_DEPENDS)

$(DOCKER_GNMI)_LOAD_DOCKERS += $(DOCKER_CONFIG_ENGINE_TRIXIE)

$(DOCKER_GNMI)_VERSION = 1.0.0
$(DOCKER_GNMI)_PACKAGE_NAME = gnmi

$(DOCKER_GNMI)_DBG_IMAGE_PACKAGES = $($(DOCKER_CONFIG_ENGINE_TRIXIE)_DBG_IMAGE_PACKAGES)

# Ensure docker-telemetry-watchdog (which uses a docker-sonic-gnmi-based image)
# is built before the docker-sonic-gnmi debug image, because the debug image build removes
# its docker-sonic-gnmi base image during cleanup.
$(DOCKER_GNMI_DBG)_AFTER += $(DOCKER_TELEMETRY_WATCHDOG)

SONIC_DOCKER_IMAGES += $(DOCKER_GNMI)
SONIC_TRIXIE_DOCKERS += $(DOCKER_GNMI)
ifeq ($(INCLUDE_SYSTEM_GNMI), y)
SONIC_INSTALL_DOCKER_IMAGES += $(DOCKER_GNMI)
endif

SONIC_DOCKER_DBG_IMAGES += $(DOCKER_GNMI_DBG)
SONIC_TRIXIE_DBG_DOCKERS += $(DOCKER_GNMI_DBG)
ifeq ($(INCLUDE_SYSTEM_GNMI), y)
SONIC_INSTALL_DOCKER_DBG_IMAGES += $(DOCKER_GNMI_DBG)
endif

$(DOCKER_GNMI)_CONTAINER_NAME = gnmi
$(DOCKER_GNMI)_RUN_OPT += -t
$(DOCKER_GNMI)_RUN_OPT += -v /etc/sonic:/etc/sonic:ro
$(DOCKER_GNMI)_RUN_OPT += -v /etc/localtime:/etc/localtime:ro
$(DOCKER_GNMI)_RUN_OPT += -v /var/run/dbus:/var/run/dbus:rw
# For disk space monitoring.
$(DOCKER_GNMI)_RUN_OPT += -v /:/mnt/host:ro
# For sonic binary image download.
$(DOCKER_GNMI)_RUN_OPT += -v /tmp:/mnt/host/tmp:rw
# For sonic binary image downloads to persistent file system.
$(DOCKER_GNMI)_RUN_OPT += -v /var/tmp:/mnt/host/var/tmp:rw
# For host command execution in gnoi.
$(DOCKER_GNMI)_RUN_OPT += --pid=host
# Container hardening: Replace --privileged with specific capabilities
$(DOCKER_GNMI)_RUN_OPT += --cap-add=SYS_ADMIN
$(DOCKER_GNMI)_RUN_OPT += --cap-add=SYS_BOOT
$(DOCKER_GNMI)_RUN_OPT += --cap-add=SYS_PTRACE
$(DOCKER_GNMI)_RUN_OPT += --cap-add=NET_ADMIN
$(DOCKER_GNMI)_RUN_OPT += --cap-add=DAC_OVERRIDE
# Security options needed for nsenter to access host namespaces from within container
$(DOCKER_GNMI)_RUN_OPT += --security-opt apparmor=unconfined
$(DOCKER_GNMI)_RUN_OPT += --security-opt seccomp=unconfined
# For GNOI running sudo command in case of container NS remapping.
$(DOCKER_GNMI)_RUN_OPT += --userns=host
# For GNMI Unix Domain Socket (local access without TLS)
$(DOCKER_GNMI)_RUN_OPT += -v /var/run/gnmi:/var/run/gnmi:rw




$(DOCKER_GNMI)_BASE_IMAGE_FILES += monit_gnmi:/etc/monit/conf.d

$(DOCKER_GNMI)_FILES += $(GNMI_RSYSLOG_CONF)

# Reuse the source-owned TARs for the native VS profile. Other configurations
# retain the existing Make package and Docker recipes above.
ifeq ($(BLDENV),trixie)
SONIC_BAZEL_SWITCHABLE_IMAGES += $(DOCKER_GNMI) $(DOCKER_GNMI_DBG)
ifeq ($(BUILD_WITH_BAZEL_WHEN_AVAILABLE):$(CONFIGURED_PLATFORM):$(CONFIGURED_ARCH):$(INCLUDE_FIPS):$(DBG_IMAGE_MARK):$(ENABLE_TRANSLIB_WRITE):$(ENABLE_NATIVE_WRITE),y:vs:amd64:y:dbg:y:y)
ifeq ($(filter y,$(CROSS_BUILD_ENVIRON) $(MULTIARCH_QEMU_ENVIRON) $(ENABLE_ASAN)),)
ifeq ($(wildcard $(DOCKERS_PATH)/$(DOCKER_GNMI_STEM)/Dockerfile.gnmi.custom.j2),)
SONIC_BAZEL_DOCKER_IMAGES += $(DOCKER_GNMI)
SONIC_BAZEL_DBG_DOCKER_IMAGES += $(DOCKER_GNMI_DBG)
$(DOCKER_GNMI)_BAZEL_TARGET = //dockers/docker-sonic-gnmi:$(DOCKER_GNMI)
$(DOCKER_GNMI_DBG)_BAZEL_TARGET = //dockers/docker-sonic-gnmi:$(DOCKER_GNMI_DBG)
$(DOCKER_GNMI)_BAZEL_ARGS = --@sonic_gnmi//tools/bazel:enable_dialout=$(if $(filter n,$(ENABLE_DIALOUT)),false,true)
$(DOCKER_GNMI_DBG)_BAZEL_ARGS = $($(DOCKER_GNMI)_BAZEL_ARGS)
SONIC_BAZEL_MANIFESTS += docker-sonic-gnmi docker-sonic-gnmi-$(DBG_IMAGE_MARK)
SONIC_BAZEL_IMAGE_ENVIRONMENTS += docker-sonic-gnmi
docker-sonic-gnmi_MANIFEST_IMAGE = $(DOCKER_GNMI)
docker-sonic-gnmi-$(DBG_IMAGE_MARK)_MANIFEST_IMAGE = $(DOCKER_GNMI)
docker-sonic-gnmi-$(DBG_IMAGE_MARK)_MANIFEST_SUFFIX = $(DBG_IMAGE_MARK)
$(DOCKER_GNMI)_BAZEL_DEPENDS = $(TARGET_PATH)/bazel-manifests/docker-base-trixie/manifest.json \
    $(TARGET_PATH)/bazel-manifests/docker-config-engine-trixie/manifest.json \
    $(TARGET_PATH)/bazel-manifests/docker-sonic-gnmi/manifest.json \
    $(TARGET_PATH)/bazel-manifests/docker-sonic-gnmi/image.env
$(DOCKER_GNMI_DBG)_BAZEL_DEPENDS = $($(DOCKER_GNMI)_BAZEL_DEPENDS) \
    $(TARGET_PATH)/bazel-manifests/docker-sonic-gnmi-$(DBG_IMAGE_MARK)/manifest.json
endif
endif
endif
endif
