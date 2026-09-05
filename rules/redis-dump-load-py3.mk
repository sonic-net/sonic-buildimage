# redis_dump_load python3 wheel

REDIS_DUMP_LOAD_PY3 = redis_dump_load-1.1-py3-none-any.whl
$(REDIS_DUMP_LOAD_PY3)_SRC_PATH = $(SRC_PATH)/redis-dump-load
$(REDIS_DUMP_LOAD_PY3)_PYTHON_VERSION = 3
ifeq ($(ENABLE_PY2_MODULES), y)
    # Synthetic dependency just to avoid race condition
    $(REDIS_DUMP_LOAD_PY3)_DEPENDS += $(REDIS_DUMP_LOAD_PY2)
endif
$(REDIS_DUMP_LOAD_PY3)_TEST = n
SONIC_PYTHON_WHEELS += $(REDIS_DUMP_LOAD_PY3)
