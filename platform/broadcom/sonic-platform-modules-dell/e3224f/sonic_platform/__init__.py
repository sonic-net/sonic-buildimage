"""
Module sonic_platform provides the platform dependent population of
platform.py, chassis.py, component.py, sfp.py, thermal.py, psu.py,
fan.py and watchdog.py
"""
__all__ = ["platform", "chassis", "sfp", "eeprom", "component", "thermal", "psu", "fan", "fan_drawer", "watchdog"]
from sonic_platform import *

