# sonic-bmpcfgd package

SONIC_BMPCFGD = sonic_bmpcfgd_services-1.0-py3-none-any.whl
$(SONIC_BMPCFGD)_SRC_PATH = $(SRC_PATH)/sonic-bmpcfgd
# These dependencies are only needed because they are dependencies
# of sonic-config-engine and bmpcfgd explicitly calls sonic-cfggen
# as part of its unit tests.
# TODO: Refactor unit tests so that these dependencies are not needed

$(SONIC_BMPCFGD)_DEPENDS += $(SONIC_CONFIG_ENGINE_PY3) \
                            $(SONIC_YANG_MGMT_PY3) \
                            $(SONIC_YANG_MODELS_PY3) \
                            $(SONIC_PY_COMMON_PY3) \
                            $(SONIC_UTILITIES_PY3)

$(SONIC_BMPCFGD)_DEBS_DEPENDS += $(LIBYANG) \
                                 $(LIBYANG_PY3) \
                                 $(LIBSWSSCOMMON) \
                                 $(PYTHON3_SWSSCOMMON)
$(SONIC_BMPCFGD)_PYTHON_VERSION = 3
SONIC_PYTHON_WHEELS += $(SONIC_BMPCFGD)
