#############################################################################
# SuperMicro SSE-T7132S
#
# Module contains an implementation of SONiC Platform Base API and
# provides the PCIe information which are available in the platform
#
#############################################################################

try:
    from sonic_platform_base.sonic_pcie.pcie_common import PcieUtil
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class Pcie(PcieUtil):
    """ T7132S Platform-specific PCIe class """
    """ fallback to pcie_common.PcieUtil to avoid pcieutil warning message """

