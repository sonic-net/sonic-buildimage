#!/usr/bin/env python3
#
# platform.py
#
# Platform implementation for Aspeed AST2700 EVB
#

try:
    from sonic_platform_aspeed_common.platform import Platform as BasePlatformClass
except ImportError as e:
    raise ImportError(str(e) + " - required module not found")


class Platform(BasePlatformClass):
    """
    Platform-specific Platform class for Aspeed AST2700 EVB
    
    Inherits from sonic_platform_aspeed_common.platform.Platform
    and can override methods as needed for EVB-specific behavior.
    """

    def __init__(self):
        super().__init__()
        # EVB-specific initialization can go here if needed

