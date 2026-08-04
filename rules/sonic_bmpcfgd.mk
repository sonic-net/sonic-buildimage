# sonic-bmpcfgd package

SONIC_BMPCFGD = sonic_bmpcfgd_services-1.0-py3-none-any.whl
$(SONIC_BMPCFGD)_SRC_PATH = $(SRC_PATH)/sonic-bmpcfgd
$(SONIC_BMPCFGD)_PYTHON_VERSION = 3
$(SONIC_BMPCFGD)_DEPENDS += $(SONIC_PY_COMMON_PY3) \
                            $(SONIC_UTILITIES_PY3)
$(SONIC_BMPCFGD)_DEBS_DEPENDS = $(LIBSWSSCOMMON) \
                                $(PYTHON3_SWSSCOMMON)
SONIC_PYTHON_WHEELS += $(SONIC_BMPCFGD)