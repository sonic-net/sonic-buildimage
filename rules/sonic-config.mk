ifeq ($(ENABLE_PY2_MODULES), y)
    # SONIC_CONFIG_ENGINE_PY2 package

    SONIC_CONFIG_ENGINE_PY2 = sonic_config_engine-1.0-py2-none-any.whl
    $(SONIC_CONFIG_ENGINE_PY2)_SRC_PATH = $(SRC_PATH)/sonic-config-engine
    $(SONIC_CONFIG_ENGINE_PY2)_DEPENDS += $(SONIC_PY_COMMON_PY2)
    $(SONIC_CONFIG_ENGINE_PY2)_DEBS_DEPENDS += $(PYTHON_SWSSCOMMON)
    $(SONIC_CONFIG_ENGINE_PY2)_PYTHON_VERSION = 2
    SONIC_PYTHON_WHEELS += $(SONIC_CONFIG_ENGINE_PY2)
endif

# SONIC_CONFIG_ENGINE_PY3 package

SONIC_CONFIG_ENGINE_PY3 = sonic_config_engine-1.0-py3-none-any.whl
$(SONIC_CONFIG_ENGINE_PY3)_SRC_PATH = $(SRC_PATH)/sonic-config-engine
$(SONIC_CONFIG_ENGINE_PY3)_DEPENDS += $(SONIC_PY_COMMON_PY3) \
                                      $(SONIC_YANG_MGMT_PY3) \
                                      $(SONIC_YANG_MODELS_PY3)
$(SONIC_CONFIG_ENGINE_PY3)_DEBS_DEPENDS += $(LIBYANG) \
                                           $(LIBYANG_CPP) \
                                           $(LIBYANG_PY3) \
	                                       $(PYTHON3_SWSSCOMMON)
ifeq ($(ENABLE_PY2_MODULES), y)
    # Synthetic dependency to avoid building the Python 2 and 3 packages
    # simultaneously and any potential conflicts which may arise
    $(SONIC_CONFIG_ENGINE_PY3)_DEPENDS += $(SONIC_CONFIG_ENGINE_PY2)
endif
$(SONIC_CONFIG_ENGINE_PY3)_PYTHON_VERSION = 3
SONIC_PYTHON_WHEELS += $(SONIC_CONFIG_ENGINE_PY3)
