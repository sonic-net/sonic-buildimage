# NVIDIA platform-utils for BMC (Aspeed platform)

include platform/mellanox/platform-utils/platform-utils.mk

# Override SRC_PATH: the include above set it to $(PLATFORM_PATH)/platform-utils
$(MELLANOX_PLATFORM_UTILS)_SRC_PATH = platform/mellanox/platform-utils

# Clear the deps inherited from the include: this wheel imports only tabulate at
# module scope, and swsscommon is used solely by the excluded bfb-installer.
$(MELLANOX_PLATFORM_UTILS)_DEPENDS =
$(MELLANOX_PLATFORM_UTILS)_DEBS_DEPENDS =

# Wheel path consumed by the BMC package's debian/rules.
export MELLANOX_PLATFORM_UTILS_WHEEL_PATH = $(abspath $(PYTHON_WHEELS_PATH)/$(MELLANOX_PLATFORM_UTILS))
