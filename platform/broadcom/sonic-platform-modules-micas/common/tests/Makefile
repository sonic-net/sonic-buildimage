PYTHON ?= python3
PYTEST_ARGS ?= -q
REPO_ROOT := $(abspath $(CURDIR)/../../../../..)
SONIC_PLATFORM_COMMON_SRC ?= $(REPO_ROOT)/src/sonic-platform-common
SONIC_PY_COMMON_SRC ?= $(REPO_ROOT)/src/sonic-py-common

.PHONY: test
test:
	PYTHONPATH="$(SONIC_PLATFORM_COMMON_SRC):$(SONIC_PY_COMMON_SRC):$(PYTHONPATH)" \
		$(PYTHON) -m pytest -c pytest.ini $(PYTEST_ARGS) .
