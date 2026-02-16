#!/usr/bin/env python3
#
# chassis.py
#
# Chassis implementation for Aspeed AST2700 EVB
#

try:
    from sonic_platform_aspeed_common.chassis import Chassis as BaseChassisClass
except ImportError as e:
    raise ImportError(str(e) + " - required module not found")


class Chassis(BaseChassisClass):
    """
    Platform-specific Chassis class for Aspeed AST2700 EVB

    Inherits from sonic_platform_aspeed_common.chassis.Chassis
    and overrides hardware configuration for EVB-specific hardware.

    Hardware Configuration (from ast2700-evb.dts):
    - 9 PWM-controlled fans (fan0-fan8)
    - 16 TACH inputs for fan speed monitoring
    - 16 ADC channels (ADC0: 8 channels, ADC1: 8 channels)
    - 2 Watchdog timers (wdt0, wdt1)
    """

    # EVB has 9 PWM-controlled fans (fan0-fan8)
    # Even though there are 16 TACH inputs, only 9 have PWM control
    NUM_FANS = 9

    # EVB has 16 ADC channels enabled (ADC0 + ADC1)
    NUM_THERMAL_SENSORS = 16

    def __init__(self):
        super().__init__()
        # EVB-specific initialization can go here if needed

