# alarmd (SONiC Alarm Monitoring Daemon) Python wheel

ALARMD = sonic_alarmd-1.0-py3-none-any.whl
$(ALARMD)_SRC_PATH = $(SRC_PATH)/alarmd
$(ALARMD)_PYTHON_VERSION = 3
$(ALARMD)_DEPENDS = $(SONIC_PY_COMMON_PY3)
$(ALARMD)_DEBS_DEPENDS = $(LIBSWSSCOMMON) $(PYTHON3_SWSSCOMMON)
SONIC_PYTHON_WHEELS += $(ALARMD)

export alarmd_py3_wheel_path="$(addprefix $(PYTHON_WHEELS_PATH)/,$(ALARMD))"
