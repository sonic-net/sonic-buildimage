# Ciena platform modules
__all__ = ["platform", "chassis", "sfp", "psu", "thermal", "fan", "fan_drawer",
           "eeprom", "components", "voltage_sensor", "current_sensor",
           "fpga_adc_sensor", "vrm_sensor", "helper"]

# Import only the platform submodule so that the standard SONiC daemon
# bootstrap pattern works:
#     import sonic_platform
#     chassis = sonic_platform.platform.Platform().get_chassis()
#
# Do NOT use "from sonic_platform import *" — the PDDF chassis base class
# performs "from sonic_platform.sfp import Sfp" (and similar) at module
# level.  A wildcard import here would trigger loading platform -> chassis
# -> pddf_chassis -> sfp while sonic_platform.__init__ is still executing,
# creating a circular-import deadlock.
from sonic_platform import platform  # noqa: E402
