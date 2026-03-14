# sonic-frr-mgmt-framework package

SONIC_FRR_MGMT_FRAMEWORK = sonic_frr_mgmt_framework-1.0-py3-none-any.whl
$(SONIC_FRR_MGMT_FRAMEWORK)_SRC_PATH = $(SRC_PATH)/sonic-frr-mgmt-framework
# These dependencies are only needed because they are dependencies
# of sonic-config-engine and frrcfgd explicitly calls sonic-cfggen
# as part of its unit tests.
# TODO: Refactor unit tests so that these dependencies are not needed

$(SONIC_FRR_MGMT_FRAMEWORK)_DEPENDS += $(SONIC_CONFIG_ENGINE_PY3)
$(SONIC_FRR_MGMT_FRAMEWORK)_DEBS_DEPENDS += $(PYTHON_SWSSCOMMON)
$(SONIC_FRR_MGMT_FRAMEWORK)_PYTHON_VERSION = 3
SONIC_PYTHON_WHEELS += $(SONIC_FRR_MGMT_FRAMEWORK)
