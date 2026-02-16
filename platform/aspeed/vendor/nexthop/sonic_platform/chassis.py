#!/usr/bin/env python3
#
# chassis.py
#
# Chassis implementation for NextHop Aspeed AST2700 BMC
#

try:
    from sonic_platform_aspeed_common.chassis import Chassis as BaseChassisClass
    from sonic_platform_aspeed_common.thermal import Thermal
    from sonic_platform_aspeed_common.watchdog import Watchdog
    from sonic_platform_base.chassis_base import ChassisBase
except ImportError as e:
    raise ImportError(str(e) + " - required module not found")


class Chassis(BaseChassisClass):
    """
    Platform-specific Chassis class for NextHop Aspeed AST2700 BMC

    Inherits from sonic_platform_aspeed_common.chassis.Chassis
    and overrides for NextHop-specific hardware configuration.

    Hardware Configuration (from nh-ast2700-r0.dts):
    - 0 PWM fans (all fan controllers disabled in DTS)
    - 0 TACH inputs (all disabled in DTS)
    - ADC controllers disabled (ADC0 and ADC1 both disabled)
    - 2 Watchdog timers (wdt0, wdt1)
    - BCM53134 managed switch with DSA configuration

    Supports multiple card revisions with runtime detection.
    """

    # NextHop has NO fans - all fan/PWM/TACH controllers are disabled in DTS
    NUM_FANS = 0

    # NextHop has ADC controllers disabled - thermal sensors may not be available
    # We'll check at runtime which sensors actually exist
    NUM_THERMAL_SENSORS = 16  # Keep same as base, but filter in __init__

    def __init__(self):
        """
        Initialize NextHop chassis with hardware-specific configuration

        Overrides base class initialization to:
        1. Skip fan initialization (no fans on NextHop)
        2. Only create thermal sensors that actually exist (ADCs disabled)
        3. Detect card revision
        """
        # Call ChassisBase.__init__ directly to skip base class fan/thermal init
        # We'll create our own filtered lists
        ChassisBase.__init__(self)

        # Initialize watchdog (same as base class)
        self._watchdog = Watchdog()

        # NextHop has NO fans - create empty lists
        self._fan_list = []
        self._fan_drawer_list = []

        # For thermal sensors, only add those that actually exist
        # ADC controllers are disabled in DTS, so most/all may not be present
        self._thermal_list = []
        for i in range(self.NUM_THERMAL_SENSORS):
            thermal = Thermal(i)
            # Only add thermal sensor if it's actually present in hardware
            if thermal.get_presence():
                self._thermal_list.append(thermal)

        # NextHop-specific initialization
        self.card_revision = self._detect_card_revision()

    def _detect_card_revision(self):
        """
        Detect the NextHop BMC card revision from hardware

        Returns:
            str: Card revision identifier (e.g., 'r0', 'r1')
        """
        # TODO: Implement revision detection from EEPROM or device tree
        # For now, default to 'r0'
        return 'r0'

