# sonic-vrrpcfgd package

SONIC_VRRPCFGD = sonic_vrrpcfgd-1.0-py3-none-any.whl
$(SONIC_VRRPCFGD)_SRC_PATH = $(SRC_PATH)/sonic-vrrpcfgd
# These dependencies are only needed because they are dependencies
# of sonic-config-engine and vrrpcfgd explicitly calls sonic-cfggen
# as part of its unit tests.
# TODO: Refactor unit tests so that these dependencies are not needed

$(SONIC_VRRPCFGD)_DEPENDS += $(SONIC_CONFIG_ENGINE_PY3)
$(SONIC_VRRPCFGD)_DEBS_DEPENDS += $(PYTHON_SWSSCOMMON)
$(SONIC_VRRPCFGD)_PYTHON_VERSION = 3
SONIC_PYTHON_WHEELS += $(SONIC_VRRPCFGD)