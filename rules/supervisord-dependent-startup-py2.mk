ifeq ($(ENABLE_PY2_MODULES), y)
    # Upstream 1.4.0 with SONiC's sidecar patches.
    SUPERVISORD_DEPENDENT_STARTUP_PY2 = supervisord_dependent_startup-1.4.0-py2-none-any.whl
    $(SUPERVISORD_DEPENDENT_STARTUP_PY2)_SRC_PATH = $(SRC_PATH)/supervisord-dependent-startup
    $(SUPERVISORD_DEPENDENT_STARTUP_PY2)_PYTHON_VERSION = 2
    SONIC_PYTHON_WHEELS += $(SUPERVISORD_DEPENDENT_STARTUP_PY2)
endif
