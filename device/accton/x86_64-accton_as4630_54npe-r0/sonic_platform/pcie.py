#############################################################################
# Edgecore
#
# Module contains an implementation of SONiC Platform Base API and
# provides the fan status which are available in the platform
# Base PCIe class
#############################################################################

try:
    from sonic_platform_base.sonic_pcie.pcie_common import PcieUtil
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class Pcie(PcieUtil):
    """Edgecore Platform-specific PCIe class"""

    def __init__(self, platform_path):
        PcieUtil.__init__(self, platform_path)