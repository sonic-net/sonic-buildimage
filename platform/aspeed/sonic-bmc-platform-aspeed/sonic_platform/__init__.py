"""
SONiC Platform API module for Aspeed BMC platform

This module provides platform-specific implementations for the SONiC Platform API.
Includes support for:
- Chassis information
- Reboot cause detection via watchdog bootstatus
- Hardware watchdog control
- Thermal sensors (ADC channels)
- Fan monitoring and control (PWM/TACH)
"""

__all__ = ['platform', 'chassis', 'watchdog', 'thermal', 'fan']

# Import submodules to make them accessible
from . import platform
from . import chassis
from . import watchdog
from . import thermal
from . import fan

