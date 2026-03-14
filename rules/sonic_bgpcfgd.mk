# sonic-bgpcfgd package

SONIC_BGPCFGD = sonic_bgpcfgd-1.0-py3-none-any.whl
$(SONIC_BGPCFGD)_SRC_PATH = $(SRC_PATH)/sonic-bgpcfgd
# These dependencies are only needed because they are dependencies
# of sonic-config-engine and bgpcfgd explicitly calls sonic-cfggen
# as part of its unit tests.
# TODO: Refactor unit tests so that these dependencies are not needed

$(SONIC_BGPCFGD)_DEPENDS += $(SONIC_CONFIG_ENGINE_PY3) \
                            $(SONIC_YANG_MGMT_PY3) \
                            $(SONIC_YANG_MODELS_PY3)
$(SONIC_BGPCFGD)_DEBS_DEPENDS += $(LIBYANG) \
                                 $(LIBYANG_CPP) \
                                 $(LIBYANG_PY3) \
                                 $(PYTHON3_SWSSCOMMON)
$(SONIC_BGPCFGD)_PYTHON_VERSION = 3
SONIC_PYTHON_WHEELS += $(SONIC_BGPCFGD)
