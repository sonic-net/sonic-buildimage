#!/usr/bin/env python

try:
    from sonic_platform_pddf_base.pddf_psu import PddfPsu
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class Psu(PddfPsu):
    """PDDF Platform-Specific PSU class for LN3848G

    The LN3848G has a single integrated PSU with no I2C monitoring path.
    All status registers are controlled by hardware; no CPLD registers
    are available for PSU presence or power-good detection.
    Methods return safe hardcoded defaults to match the original behaviour.
    """

    def __init__(self, index, pddf_data=None, pddf_plugin_data=None):
        PddfPsu.__init__(self, index, pddf_data, pddf_plugin_data)

    def get_presence(self):
        """Single integrated PSU is always present."""
        return True

    def get_powergood_status(self):
        """No I2C monitoring – assume power good."""
        return True

    def get_status(self):
        """No I2C monitoring – assume operational."""
        return True
