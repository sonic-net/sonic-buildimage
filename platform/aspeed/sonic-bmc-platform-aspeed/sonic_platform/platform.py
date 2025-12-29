"""
SONiC Platform API - Platform class for Aspeed BMC

This module provides the Platform class for Aspeed AST2700 BMC platform.
"""

try:
    from sonic_platform.chassis import Chassis
except ImportError as e:
    raise ImportError(str(e) + " - required module not found")


class Platform:
    """
    Platform class for Aspeed BMC
    
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

