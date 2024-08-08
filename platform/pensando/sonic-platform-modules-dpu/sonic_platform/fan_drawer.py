# {C} Copyright 2023 AMD Systems Inc. All rights reserved
########################################################################
# Pensando
#
# Module contains an implementation of SONiC Platform Base API and
# provides the Fan-Drawers' information available in the platform.
#
########################################################################

try:
    from sonic_platform_base.fan_drawer_base import FanDrawerBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

class FanDrawer(FanDrawerBase):
    """Platform-specific Fan class"""

    def __init__(self):
        FanDrawerBase.__init__(self)
