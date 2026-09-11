########################################################################
#
# DELLEMC Z6100
#
# Module contains a platform specific implementation of SONiC Platform
# Base PCIe class
#
########################################################################

try:
    from sonic_platform.component import Component
    from sonic_platform_base.sonic_pcie.pcie_common import PcieUtil
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class Pcie(PcieUtil):
    """DellEMC Platform-specific PCIe class"""

    def __init__(self, platform_path):
        PcieUtil.__init__(self, platform_path)
        bios = Component(component_index=0)
        bios_ver = bios.get_firmware_version()

        versions = bios_ver.split("-")
        if (len(versions) == 2) and int(versions[1], 10) > 5:
            self._conf_rev = "2"
        else:
            self._conf_rev = "1"
