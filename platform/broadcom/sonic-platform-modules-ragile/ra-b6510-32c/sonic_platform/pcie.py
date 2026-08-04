#
# pcie_base.py
#
# Abstract base class for implementing platform-specific
#  PCIE functionality for SONiC
#

try:
    import abc
    from sonic_pcie import PcieUtil
except ImportError as e:
    raise ImportError (str(e) + " - required module not found")

class PcieBase(object): 
    def __init__(self, path):
       """
         Constructor
         Args:
           pcieutil file and config file path
        """
    self.pcie_util = PcieUtil(path)


    @abc.abstractmethod
    def get_pcie_device(self):
        """
         get current device pcie info
        
          Returns:
            A list including pcie device info
         """
        return self.pcie_util.get_pcie_device()


    @abc.abstractmethod
    def get_pcie_check(self):
        """
         Check Pcie device with config file
         Returns:
            A list including pcie device and test result info
        """
        return self.pcie_util.get_pcie_check()

