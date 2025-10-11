# sonic gcu package

SONIC_GCU_PY3_VERSION = 1.0.0
SONIC_GCU_PY3_NAME = sonic_gcu
SONIC_GCU_PY3 = $(SONIC_GCU_PY3_NAME)-$(SONIC_GCU_PY3_VERSION)-py3-none-any.whl
$(SONIC_GCU_PY3)_SRC_PATH = $(SRC_PATH)/sonic-utilities/gcu
$(SONIC_GCU_PY3)_PYTHON_VERSION = 3
$(SONIC_GCU_PY3)_NAME = $(SONIC_GCU_PY3_NAME)
$(SONIC_GCU_PY3)_VERSION = $(SONIC_GCU_PY3_VERSION)
$(SONIC_GCU_PY3)_DEPENDS += $(SONIC_PY_COMMON_PY3) \
                                  $(SONIC_CONFIG_ENGINE_PY3) \
                                  $(SONIC_YANG_MGMT_PY3) \
                                  $(SONIC_YANG_MODELS_PY3)
$(SONIC_GCU_PY3)_DEBS_DEPENDS = $(LIBYANG) \
                                      $(LIBYANG_CPP) \
                                      $(LIBYANG_PY3) \
                                      $(LIBSWSSCOMMON) \
                                      $(PYTHON3_SWSSCOMMON)
SONIC_PYTHON_WHEELS += $(SONIC_GCU_PY3)
