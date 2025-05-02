#
# pcie_base.py
#
# Abstract base class for implementing platform-specific
#  PCIE functionality for SONiC
#

try:
    from sonic_platform_base.sonic_pcie.pcie_common import PcieUtil
except ImportError as e:
    raise ImportError (str(e) + " - required module not found")

class Pcie(PcieUtil):
    def __init__(self, platform_path):
        PcieUtil.__init__(self, platform_path)
