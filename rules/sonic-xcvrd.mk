# sonic-xcvrd (SONiC Transceiver monitoring daemon) Debian package

# SONIC_XCVRD_PY2 package

SONIC_XCVRD_PY2 = sonic_xcvrd-1.0-py2-none-any.whl
$(SONIC_XCVRD_PY2)_SRC_PATH = $(SRC_PATH)/sonic-platform-daemons/sonic-xcvrd
# sonic-xcvrd is nested inside the sonic-platform-daemons submodule, so the
# default "<SRC_PATH>.patch" sibling would live inside the submodule's own
# working tree and can't be tracked here. Keep patches next to the submodule
# mount instead.
$(SONIC_XCVRD_PY2)_PATCH_PATH = $(SRC_PATH)/sonic-platform-daemons.patch/sonic-xcvrd
$(SONIC_XCVRD_PY2)_DEPENDS = $(SONIC_PY_COMMON_PY2) $(SONIC_PLATFORM_COMMON_PY2)
$(SONIC_XCVRD_PY2)_DEBS_DEPENDS = $(LIBSWSSCOMMON) $(PYTHON_SWSSCOMMON)
$(SONIC_XCVRD_PY2)_PYTHON_VERSION = 2
SONIC_PYTHON_WHEELS += $(SONIC_XCVRD_PY2)

# SONIC_XCVRD_PY3 package

SONIC_XCVRD_PY3 = sonic_xcvrd-1.0-py3-none-any.whl
$(SONIC_XCVRD_PY3)_SRC_PATH = $(SRC_PATH)/sonic-platform-daemons/sonic-xcvrd
# See $(SONIC_XCVRD_PY2)_PATCH_PATH above.
$(SONIC_XCVRD_PY3)_PATCH_PATH = $(SRC_PATH)/sonic-platform-daemons.patch/sonic-xcvrd
$(SONIC_XCVRD_PY3)_DEPENDS = $(SONIC_PY_COMMON_PY3) $(SONIC_PLATFORM_COMMON_PY3)
$(SONIC_XCVRD_PY3)_DEBS_DEPENDS = $(LIBSWSSCOMMON) $(PYTHON3_SWSSCOMMON)
$(SONIC_XCVRD_PY3)_PYTHON_VERSION = 3
SONIC_PYTHON_WHEELS += $(SONIC_XCVRD_PY3)
