#!/usr/bin/env python

# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""
Unit tests for the nexthop led_control module.
These tests run in isolation from the SONiC environment using pytest:
python -m pytest test/unit/nexthop/test_led_control.py -v
"""

import pytest
from unittest.mock import MagicMock, patch

@pytest.fixture(scope="function", autouse=True)
def led_control_module():
    """Loads the module before each test. This is to let conftest.py inject deps first."""
    from nexthop import led_control

    yield led_control

class TestLedControl:
    """Test class for LED control functionality."""

    @pytest.mark.parametrize(
        "port_name, get_port_num_return, port_status_map, xcvr_presence, expected_led_device_name, expected_color",
        [
        # All interfaces up => green
        (
            "Ethernet0",
            1,
            {
                "Ethernet0": ("up", "up"),
                "Ethernet1": ("up", "up"),
                "Ethernet2": ("up", "up"),
                "Ethernet3": ("up", "up"),
                "Ethernet4": ("up", "up"),
                "Ethernet5": ("up", "up"),
                "Ethernet6": ("up", "up"),
                "Ethernet7": ("up", "up"),
            },
            True,
            "PORT_LED_1",
            "green",
        ),
        # All interfaces admin disabled => off
        (
            "Ethernet13",
            2,
            {
                "Ethernet8": ("down", "down"),
                "Ethernet9": ("down", "down"),
                "Ethernet10": ("down", "down"),
                "Ethernet11": ("down", "down"),
                "Ethernet12": ("down", "down"),
                "Ethernet13": ("down", "down"),
                "Ethernet14": ("down", "down"),
                "Ethernet15": ("down", "down"),
            },
            True,
            "PORT_LED_2",
            "off",
        ),
        # All interfaces down + xcvr not present => off
        (
            "Ethernet0",
            1,
            {
                "Ethernet0": ("up", "down"),
                "Ethernet1": ("up", "down"),
                "Ethernet2": ("up", "down"),
                "Ethernet3": ("up", "down"),
                "Ethernet4": ("up", "down"),
                "Ethernet5": ("up", "down"),
                "Ethernet6": ("up", "down"),
                "Ethernet7": ("up", "down"),
            },
            False,
            "PORT_LED_1",
            "off",
        ),
        # Unexpected interface down => yellow
        (
            "Ethernet27",
            4,
            {
                "Ethernet24": ("up", "up"),
                "Ethernet25": ("up", "up"),
                "Ethernet26": ("up", "up"),
                "Ethernet27": ("up", "down"),
                "Ethernet28": ("up", "up"),
                "Ethernet29": ("up", "up"),
                "Ethernet30": ("up", "up"),
                "Ethernet31": ("up", "up"),
            },
            True,
            "PORT_LED_4",
            "yellow",
        ),
        # Interfaces up + xcvr not present => yellow
        (
            "Ethernet0",
            1,
            {
                "Ethernet0": ("up", "up"),
                "Ethernet1": ("up", "up"),
                "Ethernet2": ("up", "up"),
                "Ethernet3": ("up", "up"),
                "Ethernet4": ("up", "up"),
                "Ethernet5": ("up", "up"),
                "Ethernet6": ("up", "up"),
                "Ethernet7": ("up", "up"),
            },
            False,
            "PORT_LED_1",
            "yellow",
        ),
        # Admin disabled interface unexpectedly up => yellow
        (
            "Ethernet0",
            1,
            {
                "Ethernet0": ("down", "up"),
                "Ethernet1": ("down", "up"),
                "Ethernet2": ("down", "up"),
                "Ethernet3": ("down", "up"),
                "Ethernet4": ("down", "up"),
                "Ethernet5": ("down", "up"),
                "Ethernet6": ("down", "up"),
                "Ethernet7": ("down", "up"),
            },
            True,
            "PORT_LED_1",
            "yellow",
        ),
        (
            "Ethernet0",
            1,
            {
                "Ethernet0": ("up", "up"),
                "Ethernet1": (None, None),
                "Ethernet2": (None, None),
                "Ethernet3": (None, None),
                "Ethernet4": (None, None),
                "Ethernet5": (None, None),
                "Ethernet6": (None, None),
                "Ethernet7": (None, None),
            },
            True,
            "PORT_LED_1",
            "green",
        ),
        (
            "Ethernet0",
            1,
            {
                "Ethernet0": ("down", "down"),
                "Ethernet1": (None, None),
                "Ethernet2": (None, None),
                "Ethernet3": (None, None),
                "Ethernet4": (None, None),
                "Ethernet5": (None, None),
                "Ethernet6": (None, None),
                "Ethernet7": (None, None),
            },
            True,
            "PORT_LED_1",
            "off",
        ),
        (
            "Ethernet0",
            1,
            {
                "Ethernet0": ("up", "down"),
                "Ethernet1": (None, None),
                "Ethernet2": (None, None),
                "Ethernet3": (None, None),
                "Ethernet4": (None, None),
                "Ethernet5": (None, None),
                "Ethernet6": (None, None),
                "Ethernet7": (None, None),
            },
            True,
            "PORT_LED_1",
            "yellow",
        ),
        (
            "Ethernet0",
            1,
            {
                "Ethernet0": (None, "down"),
                "Ethernet1": (None, None),
                "Ethernet2": (None, None),
                "Ethernet3": (None, None),
                "Ethernet4": (None, None),
                "Ethernet5": (None, None),
                "Ethernet6": (None, None),
                "Ethernet7": (None, None),
            },
            True,
            "PORT_LED_1",
            "off",
        ),
        (
            "Ethernet0",
            1,
            {
                "Ethernet0": (None, "up"),
                "Ethernet1": (None, None),
                "Ethernet2": (None, None),
                "Ethernet3": (None, None),
                "Ethernet4": (None, None),
                "Ethernet5": (None, None),
                "Ethernet6": (None, None),
                "Ethernet7": (None, None),
            },
            True,
            "PORT_LED_1",
            "yellow",
        ),
    ],
)
    def test_led_control(
        self,
        led_control_module,
        port_name,
        get_port_num_return,
        port_status_map,
        xcvr_presence,
        expected_led_device_name,
        expected_color,
    ):
        """Test LED control functionality with various port states."""
        # Get the LedControl class from the module
        LedControl = led_control_module.LedControl

        # Patch the module-level functions in the led_control module
        with patch.object(led_control_module, "get_port_config") as mock_get_port_config, \
             patch.object(led_control_module, "get_chassis", return_value=MagicMock()) as mock_get_chassis, \
             patch.object(LedControl, "_get_xcvr_presence") as mock_get_xcvr_presence, \
             patch.object(LedControl, "_get_port_status") as mock_get_port_status, \
             patch.object(LedControl, "_get_port_num") as mock_get_port_num, \
             patch.object(LedControl, "_get_interfaces_for_port") as mock_get_interfaces_for_port:
            # Mock the physical and logical port mappings
            mock_get_port_config.return_value = ({"placeholder": {}}, {}, {})
            mock_get_port_num.return_value = get_port_num_return
            mock_get_interfaces_for_port.return_value = list(port_status_map.keys())

            # Mock the admin_status and oper_status for each logical port
            def side_effect_get_port_status(logical_port):
                return port_status_map.get(logical_port)

            mock_get_port_status.side_effect = side_effect_get_port_status

            # Mock the xcvr presence check
            mock_get_xcvr_presence.return_value = xcvr_presence

            led_control = LedControl()
            led_control.port_link_state_change(port_name)

            # Assert we're setting the expected LED color
            mock_chassis = mock_get_chassis.return_value
            mock_chassis.set_system_led.assert_called_with(
                expected_led_device_name, expected_color
            )


    @pytest.mark.parametrize(
        "port_num, xcvr_info_map, expected_xcvr_presence",
        [
            (
                1,
                {
                    "Ethernet0": {"type": "OSFP 8X Pluggable Transceiver"},
                    "Ethernet4": {},
                },
                True,
            ),
            (
                2,
                {
                    "Ethernet8": {},
                    "Ethernet9": {},
                    "Ethernet10": {},
                    "Ethernet11": {},
                    "Ethernet12": {},
                    "Ethernet13": {},
                    "Ethernet14": {},
                    "Ethernet15": {},
                },
                False,
            ),
            (
                2,
                {
                    "Ethernet8": {"type": "OSFP 8X Pluggable Transceiver"},
                    "Ethernet9": {},
                    "Ethernet10": {},
                    "Ethernet11": {},
                    "Ethernet12": {},
                    "Ethernet13": {},
                    "Ethernet14": {},
                    "Ethernet15": {},
                },
                True,
            ),
            (
                65,
                {
                    "Ethernet512": {},
                },
                False,
            ),
            (
                66,
                {
                    "Ethernet513": {"type": "SFP/SFP+/SFP28"},
                },
                True,
            ),
        ],
    )
    def test_get_xcvr_presence(
        self,
        led_control_module,
        port_num,
        xcvr_info_map,
        expected_xcvr_presence,
    ):
        """Test transceiver presence detection."""
        # Get the LedControl class from the module
        LedControl = led_control_module.LedControl

        with patch.object(led_control_module, "get_port_config") as mock_get_port_config, \
             patch.object(LedControl, "_get_interfaces_for_port") as mock_get_interfaces_for_port, \
             patch.object(LedControl, "_get_xcvr_info") as mock_get_xcvr_info:

            mock_get_port_config.return_value = ({"placeholder": {}}, {}, {})
            mock_get_interfaces_for_port.return_value = list(xcvr_info_map.keys())
            mock_get_xcvr_info.side_effect = lambda port: xcvr_info_map.get(port)

            led_control = LedControl()
            xcvr_presence = led_control._get_xcvr_presence(port_num)

            assert xcvr_presence == expected_xcvr_presence

    @pytest.mark.parametrize(
        "ports_dict, expected_logical_to_physical, expected_physical_to_logical",
    [
        (
            # Default, no breakouts
            {
                "Ethernet0": {
                    "admin_status": "up",
                    "alias": "Port1",
                    "dhcp_rate_limit": "300",
                    "index": "1",
                    "lanes": "9,10,11,12,13,14,15,16",
                    "mtu": "9100",
                    "speed": "800000",
                    "subport": "0"
                },
                "Ethernet8": {
                    "admin_status": "up",
                    "alias": "Port2",
                    "dhcp_rate_limit": "300",
                    "index": "2",
                    "lanes": "1,2,3,4,5,6,7,8",
                    "mtu": "9100",
                    "speed": "800000",
                    "subport": "0"
                },
            },
            {
                "Ethernet0": 1,
                "Ethernet1": 1,
                "Ethernet2": 1,
                "Ethernet3": 1,
                "Ethernet4": 1,
                "Ethernet5": 1,
                "Ethernet6": 1,
                "Ethernet7": 1,
                "Ethernet8": 2,
                "Ethernet9": 2,
                "Ethernet10": 2,
                "Ethernet11": 2,
                "Ethernet12": 2,
                "Ethernet13": 2,
                "Ethernet14": 2,
                "Ethernet15": 2,
            },
            {
                1: [
                    "Ethernet0",
                    "Ethernet1",
                    "Ethernet2",
                    "Ethernet3",
                    "Ethernet4",
                    "Ethernet5",
                    "Ethernet6",
                    "Ethernet7",
                ],
                2: [
                    "Ethernet8",
                    "Ethernet9",
                    "Ethernet10",
                    "Ethernet11",
                    "Ethernet12",
                    "Ethernet13",
                    "Ethernet14",
                    "Ethernet15",
                ],
            },
        ),
        # SFPs
        (
            {
                "Ethernet512": {
                    "alias": "Port65",
                    "dhcp_rate_limit": "300",
                    "index": "65",
                    "lanes": "515",
                    "speed": "25000",
                    "subport": "0"
                },
                "Ethernet513": {
                    "alias": "Port66",
                    "dhcp_rate_limit": "300",
                    "index": "66",
                    "lanes": "516",
                    "speed": "25000",
                    "subport": "0"
                },
            },
            {
                "Ethernet512": 65,
                "Ethernet513": 66,
            },
            {
                65: ["Ethernet512"],
                66: ["Ethernet513"],
            },
        ),
        # Breakout example
        (
            {
                "Ethernet8": {
                    "alias": "Port2/1",
                    "dhcp_rate_limit": "300",
                    "index": "2",
                    "lanes": "1,2,3,4",
                    "speed": "400000",
                    "subport": "1"
                },
                "Ethernet12": {
                    "alias": "Port2/2",
                    "dhcp_rate_limit": "300",
                    "index": "2",
                    "lanes": "5,6,7,8",
                    "speed": "400000",
                    "subport": "2"
                },
                "Ethernet16": {
                    "alias": "Port3/1",
                    "dhcp_rate_limit": "300",
                    "index": "3",
                    "lanes": "25,26,27,28",
                    "speed": "400000",
                    "subport": "1"
                },
                "Ethernet20": {
                    "alias": "Port3/2",
                    "dhcp_rate_limit": "300",
                    "index": "3",
                    "lanes": "29,30,31,32",
                    "speed": "400000",
                    "subport": "2"
                },
            },
            {
                "Ethernet8": 2,
                "Ethernet9": 2,
                "Ethernet10": 2,
                "Ethernet11": 2,
                "Ethernet12": 2,
                "Ethernet13": 2,
                "Ethernet14": 2,
                "Ethernet15": 2,
                "Ethernet16": 3,
                "Ethernet17": 3,
                "Ethernet18": 3,
                "Ethernet19": 3,
                "Ethernet20": 3,
                "Ethernet21": 3,
                "Ethernet22": 3,
                "Ethernet23": 3,
            },
            {
                2: [
                    "Ethernet8",
                    "Ethernet9",
                    "Ethernet10",
                    "Ethernet11",
                    "Ethernet12",
                    "Ethernet13",
                    "Ethernet14",
                    "Ethernet15",
                ],
                3: [
                    "Ethernet16",
                    "Ethernet17",
                    "Ethernet18",
                    "Ethernet19",
                    "Ethernet20",
                    "Ethernet21",
                    "Ethernet22",
                    "Ethernet23",
                ],
            },
        ),
    ],
)
    def test_get_port_mappings(
        self,
        led_control_module,
        ports_dict,
        expected_logical_to_physical,
        expected_physical_to_logical,
    ):
        """Test port mapping functionality."""
        # Get the LedControl class from the module
        LedControl = led_control_module.LedControl

        with patch.object(led_control_module, "get_port_config") as mock_get_port_config:
            mock_get_port_config.return_value = (ports_dict, {}, {})

            led_control = LedControl()

            assert led_control.logical_to_physical_map == expected_logical_to_physical
            assert led_control.physical_to_logical_map == expected_physical_to_logical
