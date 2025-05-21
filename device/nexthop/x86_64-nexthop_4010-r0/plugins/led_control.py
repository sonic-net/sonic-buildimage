# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

# NH-4010 Port LED Policy

try:
    from nexthop.led_control import NexthopLedControlBase
except ImportError as e:
    raise ImportError("%s - required module not found" % e)

NUM_LANES = 8

SFP_PORT_TO_INTFS = {
    65: ["Ethernet512"],
    66: ["Ethernet513"],
}


class LedControl(NexthopLedControlBase):
    """NH-4010 specific LED control class"""

    def _get_interfaces_for_port(self, port_num):
        # SFP
        if port_num in SFP_PORT_TO_INTFS:
            return SFP_PORT_TO_INTFS[port_num]
        # OSFP
        return [
            f"Ethernet{i}"
            for i in range((port_num - 1) * NUM_LANES, port_num * NUM_LANES)
        ]
