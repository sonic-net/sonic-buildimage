ifeq ($(ENABLE_PY2_MODULES), y)
    # swsssdk python2 wheel

    SWSSSDK_PY2 = swsssdk-2.0.1-py2-none-any.whl
    $(SWSSSDK_PY2)_SRC_PATH = $(SRC_PATH)/sonic-py-swsssdk
    $(SWSSSDK_PY2)_PYTHON_VERSION = 2
    $(SWSSSDK_PY2)_DEPENDS += $(REDIS_DUMP_LOAD_PY2)
    SONIC_PYTHON_WHEELS += $(SWSSSDK_PY2)
endif
