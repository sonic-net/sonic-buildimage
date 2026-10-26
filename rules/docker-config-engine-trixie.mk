# docker image for sonic config engine

DOCKER_CONFIG_ENGINE_TRIXIE = docker-config-engine-trixie.gz
$(DOCKER_CONFIG_ENGINE_TRIXIE)_PATH = $(DOCKERS_PATH)/docker-config-engine-trixie

$(DOCKER_CONFIG_ENGINE_TRIXIE)_DEPENDS += $(LIBSWSSCOMMON) \
                                          $(LIBYANG3) \
                                          $(LIBYANG3_PY3) \
                                          $(PYTHON3_SWSSCOMMON) \
                                          $(SONIC_DB_CLI) \
                                          $(SONIC_EVENTD) \
                                          $(SONIC_SUPERVISORD_UTILITIES_RS)
$(DOCKER_CONFIG_ENGINE_TRIXIE)_PYTHON_WHEELS += $(SONIC_PY_COMMON_PY3) \
                                                  $(SONIC_YANG_MGMT_PY3) \
                                                  $(SONIC_YANG_MODELS_PY3) \
                                                  $(SONIC_CONTAINERCFGD)
$(DOCKER_CONFIG_ENGINE_TRIXIE)_PYTHON_WHEELS += $(SONIC_CONFIG_ENGINE_PY3) \
                                                  $(SONIC_SUPERVISORD_UTILITIES)
$(DOCKER_CONFIG_ENGINE_TRIXIE)_LOAD_DOCKERS += $(DOCKER_BASE_TRIXIE)
$(DOCKER_CONFIG_ENGINE_TRIXIE)_FILES += $(SWSS_VARS_TEMPLATE)
$(DOCKER_CONFIG_ENGINE_TRIXIE)_FILES += $(RSYSLOG_PLUGIN_CONF_J2)
$(DOCKER_CONFIG_ENGINE_TRIXIE)_FILES += $($(SONIC_CTRMGRD)_CONTAINER_SCRIPT)
$(DOCKER_CONFIG_ENGINE_TRIXIE)_FILES += $($(SONIC_CTRMGRD)_HEALTH_PROBE)
$(DOCKER_CONFIG_ENGINE_TRIXIE)_FILES += $($(SONIC_CTRMGRD)_STARTUP_SCRIPT)

$(DOCKER_CONFIG_ENGINE_TRIXIE)_DBG_DEPENDS = $($(DOCKER_BASE_TRIXIE)_DBG_DEPENDS) \
                                             $(LIBSWSSCOMMON_DBG) \
                                             $(LIBYANG3_DBG) \
                                             $(PYTHON3_SWSSCOMMON_DBG) \
                                             $(SONIC_DB_CLI_DBG) \
                                             $(SONIC_EVENTD_DBG)
$(DOCKER_CONFIG_ENGINE_TRIXIE)_DBG_IMAGE_PACKAGES = $($(DOCKER_BASE_TRIXIE)_DBG_IMAGE_PACKAGES)

SONIC_DOCKER_IMAGES += $(DOCKER_CONFIG_ENGINE_TRIXIE)
SONIC_TRIXIE_DOCKERS += $(DOCKER_CONFIG_ENGINE_TRIXIE)

# Source-backed OCI assembly uses the same shared Make selector and archive
# bridge as the service images. Keep legacy package metadata above for callers
# that still combine it into their own Dockerfiles.
ifeq ($(BLDENV),trixie)
SONIC_BAZEL_MANIFESTS += docker-config-engine-trixie
docker-config-engine-trixie_MANIFEST_IMAGE = $(DOCKER_CONFIG_ENGINE_TRIXIE)
SONIC_BAZEL_SWITCHABLE_IMAGES += $(DOCKER_CONFIG_ENGINE_TRIXIE)
ifeq ($(BUILD_WITH_BAZEL_WHEN_AVAILABLE),y)
ifeq ($(filter y,$(CROSS_BUILD_ENVIRON) $(MULTIARCH_QEMU_ENVIRON) $(ENABLE_ASAN)),)
SONIC_BAZEL_DOCKER_IMAGES += $(DOCKER_CONFIG_ENGINE_TRIXIE)
$(DOCKER_CONFIG_ENGINE_TRIXIE)_BAZEL_TARGET = //dockers/docker-config-engine-trixie:docker-config-engine-trixie.gz
$(DOCKER_CONFIG_ENGINE_TRIXIE)_BAZEL_DEPENDS = $(TARGET_PATH)/bazel-manifests/docker-config-engine-trixie/manifest.json
$(DOCKER_CONFIG_ENGINE_TRIXIE)_BAZEL_DEPENDS += $(TARGET_PATH)/bazel-manifests/docker-base-trixie/manifest.json
endif
endif
endif
