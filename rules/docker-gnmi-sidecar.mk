# docker image for docker-gnmi-sidecar

DOCKER_GNMI_SIDECAR_STEM = docker-gnmi-sidecar
DOCKER_GNMI_SIDECAR = $(DOCKER_GNMI_SIDECAR_STEM).gz
DOCKER_GNMI_SIDECAR_DBG = $(DOCKER_GNMI_SIDECAR_STEM)-$(DBG_IMAGE_MARK).gz

$(DOCKER_GNMI_SIDECAR)_LOAD_DOCKERS = $(DOCKER_CONFIG_ENGINE_TRIXIE)

$(DOCKER_GNMI_SIDECAR)_PATH = $(DOCKERS_PATH)/$(DOCKER_GNMI_SIDECAR_STEM)

$(DOCKER_GNMI_SIDECAR)_VERSION = 1.0.0
$(DOCKER_GNMI_SIDECAR)_PACKAGE_NAME = gnmi-sidecar

SONIC_DOCKER_IMAGES += $(DOCKER_GNMI_SIDECAR)
SONIC_TRIXIE_DOCKERS += $(DOCKER_GNMI_SIDECAR)
SONIC_INSTALL_DOCKER_IMAGES += $(DOCKER_GNMI_SIDECAR)

SONIC_DOCKER_DBG_IMAGES += $(DOCKER_GNMI_SIDECAR_DBG)
SONIC_TRIXIE_DBG_DOCKERS += $(DOCKER_GNMI_SIDECAR_DBG)
SONIC_INSTALL_DOCKER_DBG_IMAGES += $(DOCKER_GNMI_SIDECAR_DBG)


$(DOCKER_GNMI_SIDECAR)_DEPENDS += $(LIBSWSSCOMMON)

$(DOCKER_GNMI_SIDECAR)_INSTALL_PYTHON_WHEELS = $(SONIC_PY_COMMON_PY3)

$(DOCKER_GNMI_SIDECAR)_INSTALL_DEBS = $(LIBSWSSCOMMON) \
                                       $(PYTHON3_SWSSCOMMON) \
                                       $(LIBYANG3_PY3)

$(DOCKER_GNMI_SIDECAR)_CONTAINER_NAME = gnmi-sidecar
$(DOCKER_GNMI_SIDECAR)_RUN_OPT += -t --pid=host
# Container hardening: Replace --privileged with specific capabilities
$(DOCKER_GNMI_SIDECAR)_RUN_OPT += --cap-add=SYS_ADMIN
$(DOCKER_GNMI_SIDECAR)_RUN_OPT += --cap-add=SYS_PTRACE
$(DOCKER_GNMI_SIDECAR)_RUN_OPT += --cap-add=DAC_OVERRIDE
# Security options needed for nsenter to access host namespaces
$(DOCKER_GNMI_SIDECAR)_RUN_OPT += --security-opt apparmor=unconfined
$(DOCKER_GNMI_SIDECAR)_RUN_OPT += --security-opt seccomp=unconfined
$(DOCKER_GNMI_SIDECAR)_RUN_OPT += -v /lib/systemd/system:/lib/systemd/system:rw
$(DOCKER_GNMI_SIDECAR)_RUN_OPT += -v /etc/audit:/etc/audit:rw
$(DOCKER_GNMI_SIDECAR)_RUN_OPT += -v /etc/sonic:/etc/sonic:ro
$(DOCKER_GNMI_SIDECAR)_RUN_OPT += -v /etc/localtime:/etc/localtime:ro

$(DOCKER_GNMI_SIDECAR)_FILES += $(CONTAINER_CHECKER)
$(DOCKER_GNMI_SIDECAR)_FILES += $(GNMI_SYSTEMD)
$(DOCKER_GNMI_SIDECAR)_FILES += $(K8S_POD_CONTROL)
$(DOCKER_GNMI_SIDECAR)_FILES += $(GNMI_RSYSLOG_CONF)

ifeq ($(BLDENV),trixie)
SONIC_BAZEL_SWITCHABLE_IMAGES += $(DOCKER_GNMI_SIDECAR) $(DOCKER_GNMI_SIDECAR_DBG)
ifeq ($(BUILD_WITH_BAZEL_WHEN_AVAILABLE):$(CONFIGURED_PLATFORM):$(CONFIGURED_ARCH):$(INCLUDE_FIPS):$(DBG_IMAGE_MARK),y:vs:amd64:y:dbg)
ifeq ($(filter y,$(CROSS_BUILD_ENVIRON) $(MULTIARCH_QEMU_ENVIRON) $(ENABLE_ASAN)),)
SONIC_BAZEL_DOCKER_IMAGES += $(DOCKER_GNMI_SIDECAR)
SONIC_BAZEL_DBG_DOCKER_IMAGES += $(DOCKER_GNMI_SIDECAR_DBG)
$(DOCKER_GNMI_SIDECAR)_BAZEL_TARGET = //dockers/docker-gnmi-sidecar:$(DOCKER_GNMI_SIDECAR)
$(DOCKER_GNMI_SIDECAR_DBG)_PATH = $($(DOCKER_GNMI_SIDECAR)_PATH)
$(DOCKER_GNMI_SIDECAR_DBG)_BAZEL_TARGET = //dockers/docker-gnmi-sidecar:$(DOCKER_GNMI_SIDECAR_DBG)
SONIC_BAZEL_MANIFESTS += docker-gnmi-sidecar docker-gnmi-sidecar-dbg
docker-gnmi-sidecar_MANIFEST_IMAGE = $(DOCKER_GNMI_SIDECAR)
docker-gnmi-sidecar-dbg_MANIFEST_IMAGE = $(DOCKER_GNMI_SIDECAR)
docker-gnmi-sidecar-dbg_MANIFEST_SUFFIX = dbg
$(DOCKER_GNMI_SIDECAR)_BAZEL_DEPENDS = $(TARGET_PATH)/bazel-manifests/docker-base-trixie/manifest.json \
    $(TARGET_PATH)/bazel-manifests/docker-config-engine-trixie/manifest.json \
    $(TARGET_PATH)/bazel-manifests/docker-gnmi-sidecar/manifest.json \
    $(TARGET_PATH)/bazel-manifests/docker-gnmi-sidecar/image.env
$(DOCKER_GNMI_SIDECAR_DBG)_BAZEL_DEPENDS = $($(DOCKER_GNMI_SIDECAR)_BAZEL_DEPENDS) \
    $(TARGET_PATH)/bazel-manifests/docker-gnmi-sidecar-dbg/manifest.json
SONIC_BAZEL_IMAGE_ENVIRONMENTS += docker-gnmi-sidecar
endif
endif
endif
