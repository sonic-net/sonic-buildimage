from nexthop.led_control import NexthopLedControlBase

NUM_LANES = 8

SFP_PORT_TO_INTFS = {
    65: ["Ethernet512"],
    66: ["Ethernet513"],
}


class ExampleLedControl(NexthopLedControlBase):
    """Example platform specific LED control class for unit testing"""

    def _get_interfaces_for_port(self, port_num):
        if port_num in SFP_PORT_TO_INTFS:
            return SFP_PORT_TO_INTFS[port_num]
        return [
            f"Ethernet{i}"
            for i in range((port_num - 1) * NUM_LANES, port_num * NUM_LANES)
        ]
