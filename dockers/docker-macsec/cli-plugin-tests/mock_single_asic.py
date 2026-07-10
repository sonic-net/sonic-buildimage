# MONKEY PATCH!!!
from unittest import mock

from sonic_py_common import multi_asic
from utilities_common import multi_asic as multi_asic_util

mock_intf_table = {
    '': {
        'eth0': {
            2: [{'addr': '10.1.1.1', 'netmask': '255.255.255.0', 'broadcast': '10.1.1.1'}],
            10: [{'addr': '3100::1', 'netmask': 'ffff:ffff:ffff:ffff::/64'}]
        },
        'Ethernet0': {
            17: [{'addr': '82:fd:d1:5b:45:2f', 'broadcast': 'ff:ff:ff:ff:ff:ff'}],
            2: [
                    {'addr': '20.1.1.1', 'netmask': '255.255.255.0', 'broadcast': '20.1.1.1'},
                    {'addr': '21.1.1.1', 'netmask': '255.255.255.0', 'broadcast': '21.1.1.1'}
                ],
            10: [
                    {'addr': 'aa00::1', 'netmask': 'ffff:ffff:ffff:ffff::/64'},
                    {'addr': '2100::1', 'netmask': 'ffff:ffff:ffff:ffff::/64'},
                    {'addr': 'fe80::64be:a1ff:fe85:c6c4%Ethernet0', 'netmask': 'ffff:ffff:ffff:ffff::/64'}
                ]
        },
        'PortChannel0001': {
            17: [{'addr': '82:fd:d1:5b:45:2f', 'broadcast': 'ff:ff:ff:ff:ff:ff'}], 
            2: [{'addr': '30.1.1.1', 'netmask': '255.255.255.0', 'broadcast': '30.1.1.1'}],
            10: [
                    {'addr': 'ab00::1', 'netmask': 'ffff:ffff:ffff:ffff::/64'},
                    {'addr': 'fe80::cc8d:60ff:fe08:139f%PortChannel0001', 'netmask': 'ffff:ffff:ffff:ffff::/64'}
                ]
        },
        'Vlan100': {
            17: [{'addr': '82:fd:d1:5b:45:2f', 'broadcast': 'ff:ff:ff:ff:ff:ff'}], 
            2: [{'addr': '40.1.1.1', 'netmask': '255.255.255.0', 'broadcast': '30.1.1.1'}],
            10: [
                    {'addr': 'cc00::1', 'netmask': 'ffff:ffff:ffff:ffff::/64'},
                    {'addr': 'fe80::c029:3fff:fe41:cf56%Vlan100', 'netmask': 'ffff:ffff:ffff:ffff::/64'}
                ]
        },
        'lo': {
            2: [{'addr': '127.0.0.1', 'netmask': '255.0.0.0', 'broadcast': '127.255.255.255'}],
            10: [{'addr': '::1', 'netmask':'ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff/128'}]
        }
    }
}


def mock_get_num_asics():
    return 1

def mock_is_multi_asic():
    return False

def mock_get_namespace_list(namespace=None):
    return ['']


def mock_single_asic_get_ip_intf_from_ns(namespace):
    interfaces = []
    try:
        interfaces = list(mock_intf_table[namespace].keys())
    except KeyError:
        pass
    return interfaces


def mock_single_asic_get_ip_intf_addr_from_ns(namespace, iface):
    ipaddresses = []
    try:
        ipaddresses = mock_intf_table[namespace][iface]
    except KeyError:
        pass
    return ipaddresses


multi_asic.is_multi_asic = mock_is_multi_asic
multi_asic.get_num_asics = mock_get_num_asics
multi_asic.get_namespace_list = mock_get_namespace_list
multi_asic_util.multi_asic_get_ip_intf_from_ns = mock_single_asic_get_ip_intf_from_ns
multi_asic_util.multi_asic_get_ip_intf_addr_from_ns = mock_single_asic_get_ip_intf_addr_from_ns
