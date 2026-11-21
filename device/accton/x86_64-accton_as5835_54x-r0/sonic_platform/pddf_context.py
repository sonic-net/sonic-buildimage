"""PDDF environmental data over the existing AS5835 kernel drivers.

The native platform service continues to own I2C client creation, LEDs,
transceivers and the thermal policy. This module only loads API mappings.
"""
import json
from sonic_platform_pddf_base.pddfapi import PddfApi


def load_environment():
    # PddfApi creates the host platform link before loading its device data.
    pddf = PddfApi()
    with open('/usr/share/sonic/platform/pddf/pd-plugin.json') as stream:
        plugin = json.load(stream)
    return pddf, plugin
