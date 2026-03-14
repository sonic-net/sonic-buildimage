# sonic-pcied (SONiC PCIe Monitor daemon) Debian package

# SONIC_PCIED_PY3 package

SONIC_PCIED_PY3 = sonic_pcied-1.0-py3-none-any.whl
$(SONIC_PCIED_PY3)_SRC_PATH = $(SRC_PATH)/sonic-platform-daemons/sonic-pcied
$(SONIC_PCIED_PY3)_DEPENDS = $(SONIC_PY_COMMON_PY3)
$(SONIC_PCIED_PY3)_PYTHON_VERSION = 3
SONIC_PYTHON_WHEELS += $(SONIC_PCIED_PY3)
