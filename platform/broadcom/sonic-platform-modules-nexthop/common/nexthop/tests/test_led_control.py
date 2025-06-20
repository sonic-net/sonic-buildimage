import os
import sys
from unittest.mock import MagicMock, patch

import pytest

sys.modules["sonic_platform"] = MagicMock()
sys.modules["sonic_platform.platform"] = MagicMock()
sys.modules["sonic_platform_pddf_base.pddf_chassis"] = MagicMock()
sys.modules["swsscommon.swsscommon"] = MagicMock()

CWD = os.path.dirname(os.path.realpath(__file__))
sys.path.append(CWD)
sys.path.append(os.path.join(CWD, "../"))
sys.path.append(os.path.join(CWD, "../../"))
from example_led_control import ExampleLedControl


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
                "Ethernet16": ("down", "down"),
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
@patch.object(ExampleLedControl, "_get_xcvr_presence")
@patch.object(ExampleLedControl, "_get_port_status")
@patch.object(ExampleLedControl, "_get_port_num")
@patch("nexthop.led_control.get_chassis", return_value=MagicMock())
def test_led_control(
    mock_get_chassis,
    mock_get_port_num,
    mock_get_port_status,
    mock_get_xcvr_presence,
    port_name,
    get_port_num_return,
    port_status_map,
    xcvr_presence,
    expected_led_device_name,
    expected_color,
):
    # Mock the physical port number we read from CONFIG_DB for a given logical port
    mock_get_port_num.return_value = get_port_num_return

    # Mock the admin_status and oper_status for each logical port
    def side_effect_get_port_status(logical_port):
        return port_status_map.get(logical_port)

    mock_get_port_status.side_effect = side_effect_get_port_status

    # Mock the xcvr presence check
    mock_get_xcvr_presence.return_value = xcvr_presence

    led_control = ExampleLedControl()
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
@patch.object(ExampleLedControl, "_get_xcvr_info")
def test_get_xcvr_presence(
    mock_get_xcvr_info,
    port_num,
    xcvr_info_map,
    expected_xcvr_presence,
):
    mock_get_xcvr_info.side_effect = lambda port: xcvr_info_map.get(port)

    led_control = ExampleLedControl()
    xcvr_presence = led_control._get_xcvr_presence(port_num)

    assert xcvr_presence == expected_xcvr_presence
