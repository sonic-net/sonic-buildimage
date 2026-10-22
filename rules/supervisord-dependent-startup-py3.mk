# Upstream 1.4.0 with SONiC's sidecar patches.
SUPERVISORD_DEPENDENT_STARTUP_PY3 = supervisord_dependent_startup-1.4.0-py3-none-any.whl
$(SUPERVISORD_DEPENDENT_STARTUP_PY3)_SRC_PATH = $(SRC_PATH)/supervisord-dependent-startup
$(SUPERVISORD_DEPENDENT_STARTUP_PY3)_PYTHON_VERSION = 3
ifeq ($(ENABLE_PY2_MODULES), y)
    # Serialize builds that patch the same source tree.
    $(SUPERVISORD_DEPENDENT_STARTUP_PY3)_DEPENDS += $(SUPERVISORD_DEPENDENT_STARTUP_PY2)
endif
# Upstream has tests_require but no testing extra for the modern wheel recipe.
$(SUPERVISORD_DEPENDENT_STARTUP_PY3)_TEST = n
SONIC_PYTHON_WHEELS += $(SUPERVISORD_DEPENDENT_STARTUP_PY3)
