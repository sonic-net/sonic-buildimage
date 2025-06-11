#!/usr/bin/env python
#


try:
    from sonic_platform_pddf_base.pddf_current_sensor import PddfCurrentSensor
except ImportError as e:
    raise ImportError (str(e) + "- required module not found")

class CurrentSensor(PddfCurrentSensor):
    """PDDF Platform-Specific class"""

    def __init__(self, index, pddf_data=None, pddf_plugin_data=None):
        PddfCurrentSensor.__init__(self, index, pddf_data, pddf_plugin_data)        
    # Provide the functions/variables below for which implementation is to be overwritten
