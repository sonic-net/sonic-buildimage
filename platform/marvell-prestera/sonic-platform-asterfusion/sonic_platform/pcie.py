############################################################################
# Asterfusion CX-N Devices PCIE                                            #
#                                                                          #
# Platform and model specific pcie subclass, inherits from the base class, #
# provides the pcie info which are available in the platform               #
#                                                                          #
############################################################################

try:
    from .constants import *
    from .helper import Helper
    from .logger import Logger

    from sonic_platform_base.sonic_pcie.pcie_common import PcieUtil
except ImportError as err:
    raise ImportError(str(err) + "- required module not found")


class Pcie(PcieUtil):
    """Platform-specific EEPROM class"""

    def __init__(self, path):
        self._helper = Helper()
        self._logger = Logger()
        PcieUtil.__init__(self, path)
