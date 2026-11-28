# docker image for docker-restapi-sidecar

DOCKER_RESTAPI_SIDECAR_STEM = docker-restapi-sidecar
DOCKER_RESTAPI_SIDECAR = $(DOCKER_RESTAPI_SIDECAR_STEM).gz
DOCKER_RESTAPI_SIDECAR_DBG = $(DOCKER_RESTAPI_SIDECAR_STEM)-$(DBG_IMAGE_MARK).gz

$(DOCKER_RESTAPI_SIDECAR)_LOAD_DOCKERS = $(DOCKER_CONFIG_ENGINE_TRIXIE)

$(DOCKER_RESTAPI_SIDECAR)_PATH = $(DOCKERS_PATH)/$(DOCKER_RESTAPI_SIDECAR_STEM)

$(DOCKER_RESTAPI_SIDECAR)_VERSION = 1.0.0
$(DOCKER_RESTAPI_SIDECAR)_PACKAGE_NAME = restapi-sidecar

SONIC_DOCKER_IMAGES += $(DOCKER_RESTAPI_SIDECAR)
SONIC_TRIXIE_DOCKERS += $(DOCKER_RESTAPI_SIDECAR)
SONIC_INSTALL_DOCKER_IMAGES += $(DOCKER_RESTAPI_SIDECAR)

SONIC_DOCKER_DBG_IMAGES += $(DOCKER_RESTAPI_SIDECAR_DBG)
SONIC_TRIXIE_DBG_DOCKERS += $(DOCKER_RESTAPI_SIDECAR_DBG)
SONIC_INSTALL_DOCKER_DBG_IMAGES += $(DOCKER_RESTAPI_SIDECAR_DBG)


$(DOCKER_RESTAPI_SIDECAR)_INSTALL_PYTHON_WHEELS = $(SONIC_PY_COMMON_PY3)

$(DOCKER_RESTAPI_SIDECAR)_CONTAINER_NAME = restapi-sidecar
# NOTE: This container must run in privileged mode with host PID namespace so that
# nsenter can access host namespaces and systemd-related scripts/files can be
# synchronized with the host system. This grants broad host access and should be
# restricted to environments where this level of privilege is acceptable.
$(DOCKER_RESTAPI_SIDECAR)_RUN_OPT += -t --privileged --pid=host
$(DOCKER_RESTAPI_SIDECAR)_RUN_OPT += -v /lib/systemd/system:/lib/systemd/system:rw
$(DOCKER_RESTAPI_SIDECAR)_RUN_OPT += -v /etc/sonic:/etc/sonic:ro
$(DOCKER_RESTAPI_SIDECAR)_RUN_OPT += -v /etc/localtime:/etc/localtime:ro

$(DOCKER_RESTAPI_SIDECAR)_FILES += $(CONTAINER_CHECKER)
$(DOCKER_RESTAPI_SIDECAR)_FILES += $(RESTAPI_SYSTEMD)
$(DOCKER_RESTAPI_SIDECAR)_FILES += $(K8S_POD_CONTROL)


ifeq ($(BLDENV),trixie)
SONIC_BAZEL_SWITCHABLE_IMAGES += $(DOCKER_RESTAPI_SIDECAR) $(DOCKER_RESTAPI_SIDECAR_DBG)
ifeq ($(BUILD_WITH_BAZEL_WHEN_AVAILABLE):$(CONFIGURED_PLATFORM):$(CONFIGURED_ARCH):$(INCLUDE_FIPS):$(DBG_IMAGE_MARK),y:vs:amd64:y:dbg)
ifeq ($(filter y,$(CROSS_BUILD_ENVIRON) $(MULTIARCH_QEMU_ENVIRON) $(ENABLE_ASAN)),)
SONIC_BAZEL_DOCKER_IMAGES += $(DOCKER_RESTAPI_SIDECAR)
SONIC_BAZEL_DBG_DOCKER_IMAGES += $(DOCKER_RESTAPI_SIDECAR_DBG)
$(DOCKER_RESTAPI_SIDECAR)_BAZEL_TARGET = //dockers/docker-restapi-sidecar:$(DOCKER_RESTAPI_SIDECAR)
$(DOCKER_RESTAPI_SIDECAR_DBG)_PATH = $($(DOCKER_RESTAPI_SIDECAR)_PATH)
$(DOCKER_RESTAPI_SIDECAR_DBG)_BAZEL_TARGET = //dockers/docker-restapi-sidecar:$(DOCKER_RESTAPI_SIDECAR_DBG)
SONIC_BAZEL_MANIFESTS += docker-restapi-sidecar docker-restapi-sidecar-dbg
docker-restapi-sidecar_MANIFEST_IMAGE = $(DOCKER_RESTAPI_SIDECAR)
docker-restapi-sidecar-dbg_MANIFEST_IMAGE = $(DOCKER_RESTAPI_SIDECAR)
docker-restapi-sidecar-dbg_MANIFEST_SUFFIX = dbg
$(DOCKER_RESTAPI_SIDECAR)_BAZEL_DEPENDS = $(TARGET_PATH)/bazel-manifests/docker-base-trixie/manifest.json \
    $(TARGET_PATH)/bazel-manifests/docker-config-engine-trixie/manifest.json \
    $(TARGET_PATH)/bazel-manifests/docker-restapi-sidecar/manifest.json \
    $(TARGET_PATH)/bazel-manifests/docker-restapi-sidecar/image.env
$(DOCKER_RESTAPI_SIDECAR_DBG)_BAZEL_DEPENDS = $($(DOCKER_RESTAPI_SIDECAR)_BAZEL_DEPENDS) \
    $(TARGET_PATH)/bazel-manifests/docker-restapi-sidecar-dbg/manifest.json
SONIC_BAZEL_IMAGE_ENVIRONMENTS += docker-restapi-sidecar
endif
endif
endif
