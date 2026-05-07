#!/usr/bin/env python

try:
    from sonic_platform_pddf_base.pddf_current_sensor import PddfCurrentSensor
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class CurrentSensor(PddfCurrentSensor):
    """PDDF Generic CurrentSensor class"""

    def __init__(self, index, pddf_data=None, pddf_plugin_data=None):
        PddfCurrentSensor.__init__(self, index, pddf_data, pddf_plugin_data)