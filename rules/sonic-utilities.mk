# sonic utilities package

SONIC_UTILITIES_PY3_VERSION = 1.2
SONIC_UTILITIES_PY3_NAME = sonic_utilities
SONIC_UTILITIES_PY3 = $(SONIC_UTILITIES_PY3_NAME)-$(SONIC_UTILITIES_PY3_VERSION)-py3-none-any.whl
$(SONIC_UTILITIES_PY3)_SRC_PATH = $(SRC_PATH)/sonic-utilities
$(SONIC_UTILITIES_PY3)_PYTHON_VERSION = 3
$(SONIC_UTILITIES_PY3)_NAME = $(SONIC_UTILITIES_PY3_NAME)
$(SONIC_UTILITIES_PY3)_VERSION = $(SONIC_UTILITIES_PY3_VERSION)
$(SONIC_UTILITIES_PY3)_DEPENDS += $(SONIC_PY_COMMON_PY3) \
                                  $(SONIC_CONFIG_ENGINE_PY3) \
                                  $(SONIC_PLATFORM_COMMON_PY3) \
                                  $(SONIC_YANG_MGMT_PY3) \
                                  $(SONIC_YANG_MODELS_PY3)
$(SONIC_UTILITIES_PY3)_DEBS_DEPENDS = $(LIBYANG) \
                                      $(LIBYANG_CPP) \
                                      $(LIBYANG_PY3) \
                                      $(LIBSWSSCOMMON) \
                                      $(PYTHON3_SWSSCOMMON)
ifeq ($(CONFIGURED_PLATFORM),nvidia-bluefield)
$(SONIC_UTILITIES_PY3)_DEBS_DEPENDS += $(LIB_SONIC_DASH_API)
endif
SONIC_PYTHON_WHEELS += $(SONIC_UTILITIES_PY3)
