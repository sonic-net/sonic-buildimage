# SONiC host services package

SONIC_HOST_SERVICES_PY3 = sonic_host_services-1.0-py3-none-any.whl
$(SONIC_HOST_SERVICES_PY3)_SRC_PATH = $(SRC_PATH)/sonic-host-services
$(SONIC_HOST_SERVICES_PY3)_PYTHON_VERSION = 3
$(SONIC_HOST_SERVICES_PY3)_DEPENDS += $(SONIC_PY_COMMON_PY3) \
                                      $(SONIC_UTILITIES_PY3)
$(SONIC_HOST_SERVICES_PY3)_DEBS_DEPENDS = $(LIBSWSSCOMMON) \
                                          $(PYTHON3_SWSSCOMMON)
SONIC_PYTHON_WHEELS += $(SONIC_HOST_SERVICES_PY3)
