# docker image for sonic config engine

DOCKER_CONFIG_ENGINE_RESOLUTE = docker-config-engine-resolute.gz
$(DOCKER_CONFIG_ENGINE_RESOLUTE)_PATH = $(DOCKERS_PATH)/docker-config-engine-resolute

$(DOCKER_CONFIG_ENGINE_RESOLUTE)_DEPENDS += $(LIBSWSSCOMMON) \
                                          $(LIBYANG3) \
                                          $(LIBYANG3_PY3) \
                                          $(PYTHON3_SWSSCOMMON) \
                                          $(SONIC_DB_CLI) \
                                          $(SONIC_EVENTD) \
                                          $(SONIC_SUPERVISORD_UTILITIES_RS)
$(DOCKER_CONFIG_ENGINE_RESOLUTE)_PYTHON_WHEELS += $(SONIC_PY_COMMON_PY3) \
                                                  $(SONIC_YANG_MGMT_PY3) \
                                                  $(SONIC_YANG_MODELS_PY3) \
                                                  $(SONIC_CONTAINERCFGD)
$(DOCKER_CONFIG_ENGINE_RESOLUTE)_PYTHON_WHEELS += $(SONIC_CONFIG_ENGINE_PY3) \
                                                  $(SONIC_SUPERVISORD_UTILITIES)
$(DOCKER_CONFIG_ENGINE_RESOLUTE)_LOAD_DOCKERS += $(DOCKER_BASE_RESOLUTE)
$(DOCKER_CONFIG_ENGINE_RESOLUTE)_FILES += $(SWSS_VARS_TEMPLATE)
$(DOCKER_CONFIG_ENGINE_RESOLUTE)_FILES += $(RSYSLOG_PLUGIN_CONF_J2)
$(DOCKER_CONFIG_ENGINE_RESOLUTE)_FILES += $($(SONIC_CTRMGRD)_CONTAINER_SCRIPT)
$(DOCKER_CONFIG_ENGINE_RESOLUTE)_FILES += $($(SONIC_CTRMGRD)_HEALTH_PROBE)
$(DOCKER_CONFIG_ENGINE_RESOLUTE)_FILES += $($(SONIC_CTRMGRD)_STARTUP_SCRIPT)

$(DOCKER_CONFIG_ENGINE_RESOLUTE)_DBG_DEPENDS = $($(DOCKER_BASE_RESOLUTE)_DBG_DEPENDS) \
                                             $(LIBSWSSCOMMON_DBG) \
                                             $(LIBYANG3_DBG) \
                                             $(PYTHON3_SWSSCOMMON_DBG) \
                                             $(SONIC_DB_CLI_DBG) \
                                             $(SONIC_EVENTD_DBG)
$(DOCKER_CONFIG_ENGINE_RESOLUTE)_DBG_IMAGE_PACKAGES = $($(DOCKER_BASE_RESOLUTE)_DBG_IMAGE_PACKAGES)

SONIC_DOCKER_IMAGES += $(DOCKER_CONFIG_ENGINE_RESOLUTE)
SONIC_RESOLUTE_DOCKERS += $(DOCKER_CONFIG_ENGINE_RESOLUTE)
