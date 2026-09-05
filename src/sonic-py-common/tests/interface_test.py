import os
import sys

from sonic_py_common import interface
from sonic_py_common.multi_asic import is_front_panel_port

class TestInterface(object):
    def test_get_interface_table_name(self):
        result = interface.get_interface_table_name("Ethernet0")
        assert result == "INTERFACE"
        result = interface.get_interface_table_name("Ethernet0.100")
        assert result == "VLAN_SUB_INTERFACE"
        result = interface.get_interface_table_name("PortChannel0")
        assert result == "PORTCHANNEL_INTERFACE"
        result = interface.get_interface_table_name("PortChannel0.100")
        assert result == "VLAN_SUB_INTERFACE"
        result = interface.get_interface_table_name("Vlan100")
        assert result == "VLAN_INTERFACE"
        result = interface.get_interface_table_name("Loopback0")
        assert result == "LOOPBACK_INTERFACE"
        result = interface.get_interface_table_name("Eth0.1001")
        assert result == "VLAN_SUB_INTERFACE"
        result = interface.get_interface_table_name("Po0.1001")
        assert result == "VLAN_SUB_INTERFACE"

    def test_get_port_table_name(self):
        result = interface.get_port_table_name("Ethernet0")
        assert result == "PORT"
        result = interface.get_port_table_name("Ethernet0.100")
        assert result == "VLAN_SUB_INTERFACE"
        result = interface.get_port_table_name("PortChannel0")
        assert result == "PORTCHANNEL"
        result = interface.get_port_table_name("PortChannel0.100")
        assert result == "VLAN_SUB_INTERFACE"
        result = interface.get_port_table_name("Vlan100")
        assert result == "VLAN_INTERFACE"
        result = interface.get_port_table_name("Loopback0")
        assert result == "LOOPBACK_INTERFACE"
        result = interface.get_port_table_name("Eth0.1001")
        assert result == "VLAN_SUB_INTERFACE"
        result = interface.get_port_table_name("Po0.1001")
        assert result == "VLAN_SUB_INTERFACE"

    def test_verify_front_panel_api(self):
        assert is_front_panel_port("Ethernet0")
        assert not is_front_panel_port("Ethernet-BP")
        assert not is_front_panel_port("Ethernet-IB")
        assert not is_front_panel_port("Ethernet-Rec")
        assert not is_front_panel_port("Ethernet254", "Dpc")
        assert not is_front_panel_port("Ethernet254", "Int")
        assert is_front_panel_port("Ethernet254", "Ext")
        assert not is_front_panel_port("Ethernet254.30")
        assert not is_front_panel_port("PortConfigDone")
