ifeq ($(ENABLE_PY2_MODULES), y)
    # redis_dump_load python2 wheel

    REDIS_DUMP_LOAD_PY2 = redis_dump_load-1.1-py2-none-any.whl
    $(REDIS_DUMP_LOAD_PY2)_SRC_PATH = $(SRC_PATH)/redis-dump-load
    $(REDIS_DUMP_LOAD_PY2)_PYTHON_VERSION = 2
    SONIC_PYTHON_WHEELS += $(REDIS_DUMP_LOAD_PY2)
endif
