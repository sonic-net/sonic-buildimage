try:
    from sonic_platform_base.sonic_pcie.pcie_common import PcieUtil
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

class Pcie(PcieUtil):
    """Celestica Platform-specific PCIe class"""

    def __init__(self, platform_path):
        super().__init__(platform_path)
