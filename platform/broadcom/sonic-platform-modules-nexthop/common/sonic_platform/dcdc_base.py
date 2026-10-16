"""
    dcdc_base.py

    Abstract base class for implementing a platform-specific class with which
    to interact with a DC/DC device in SONiC
"""

from sonic_platform_base.device_base import DeviceBase
from sonic_platform.blackbox_base import BlackBoxBase

class DcdcBase(DeviceBase, BlackBoxBase):
    """
    Abstract base class for interfacing with a DC/DC device
    """

    DEVICE_TYPE = "dcdc"
