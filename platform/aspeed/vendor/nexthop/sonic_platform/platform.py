#!/usr/bin/env python3
#
# platform.py
#
# Platform implementation for NextHop Aspeed AST2700 BMC
#

try:
    from sonic_platform_aspeed_common.platform import Platform as BasePlatformClass
except ImportError as e:
    raise ImportError(str(e) + " - required module not found")


class Platform(BasePlatformClass):
    """
    Platform-specific Platform class for NextHop Aspeed AST2700 BMC
    
    Inherits from sonic_platform_aspeed_common.platform.Platform
    and can override methods as needed for NextHop-specific behavior.
    """

    def __init__(self):
        super().__init__()
        # NextHop-specific initialization can go here if needed

