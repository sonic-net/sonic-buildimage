"""
SONiC Platform API - Platform class for Nexthop B27 BMC
"""

try:
    from sonic_platform.chassis import Chassis
except ImportError as e:
    raise ImportError(str(e) + " - required module not found")


class Platform:
    """
    Platform class for Nexthop B27 BMC
    
    Provides access to chassis-level functionality.
    """
    
    def __init__(self):
        """
        Initialize the Platform object
        """
        self._chassis = Chassis()
    
    def get_chassis(self):
        """
        Retrieves the chassis object
        
        Returns:
            An object derived from ChassisBase representing the chassis
        """
        return self._chassis

