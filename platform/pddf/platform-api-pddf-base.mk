####################################################
# PDDF Generic 2.0 Platform API base classes
####################################################
PDDF_PLATFORM_API_BASE_VERSION = 1.0

export PDDF_PLATFORM_API_BASE_VERSION

ifeq ($(ENABLE_PY2_MODULES), y)
    PDDF_PLATFORM_API_BASE_PY2 = sonic_platform_pddf_common-$(PDDF_PLATFORM_API_BASE_VERSION)-py2-none-any.whl
    $(PDDF_PLATFORM_API_BASE_PY2)_SRC_PATH = $(PLATFORM_PDDF_PATH)/platform-api-pddf-base
    $(PDDF_PLATFORM_API_BASE_PY2)_PYTHON_VERSION = 2
    $(PDDF_PLATFORM_API_BASE_PY2)_DEPENDS = $(SONIC_CONFIG_ENGINE)
    SONIC_PYTHON_WHEELS += $(PDDF_PLATFORM_API_BASE_PY2)

    export pddf_platform_api_base_py2_wheel_path="$(addprefix $(PYTHON_WHEELS_PATH)/,$(PDDF_PLATFORM_API_BASE_PY2))"
    export PDDF_PLATFORM_API_BASE_PY2
endif

PDDF_PLATFORM_API_BASE_PY3 = sonic_platform_pddf_common-$(PDDF_PLATFORM_API_BASE_VERSION)-py3-none-any.whl
$(PDDF_PLATFORM_API_BASE_PY3)_SRC_PATH = $(PLATFORM_PDDF_PATH)/platform-api-pddf-base
$(PDDF_PLATFORM_API_BASE_PY3)_PYTHON_VERSION = 3
$(PDDF_PLATFORM_API_BASE_PY3)_DEPENDS = $(SONIC_CONFIG_ENGINE)
ifeq ($(ENABLE_PY2_MODULES), y)
    # Synthetic dependency to avoid building the Python 2 and 3 packages
    # simultaneously and any potential conflicts which may arise
    $(PDDF_PLATFORM_API_BASE_PY3)_DEPENDS += $(PDDF_PLATFORM_API_BASE_PY2)
endif
$(PDDF_PLATFORM_API_BASE_PY3)_TEST = n
SONIC_PYTHON_WHEELS += $(PDDF_PLATFORM_API_BASE_PY3)

export pddf_platform_api_base_py3_wheel_path="$(addprefix $(PYTHON_WHEELS_PATH)/,$(PDDF_PLATFORM_API_BASE_PY3))"
export PDDF_PLATFORM_API_BASE_PY3
