#!/usr/bin/env python


try:
    import subprocess
    from sonic_platform_pddf_base.pddf_voltage_sensor import PddfVoltageSensor
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class VoltageSensor(PddfVoltageSensor):
    """PDDF Platform-Specific Voltage class"""

    def __init__(self, index, pddf_data=None, pddf_plugin_data=None):
        # idx is 0-based
        PddfVoltageSensor.__init__(self, index, pddf_data, pddf_plugin_data)

