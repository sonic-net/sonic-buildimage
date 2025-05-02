import json
import pytest
import subprocess
import os

import tests.common_utils as utils
from sonic_py_common.general import getstatusoutput_noshell


#TODO: Remove this fixuture once SONiC moves to python3.x
@pytest.fixture(scope="class")
def is_test_supported():
    if not utils.PY3x:
        pytest.skip('module not support in python2')
    else:
        pass


@pytest.mark.usefixtures("is_test_supported")
class TestCfgGen(object):

    @pytest.fixture(autouse=True)
    def setup_teardown(self):
        self.test_dir = os.path.dirname(os.path.realpath(__file__))
        self.script_file = [utils.PYTHON_INTERPRETTER, os.path.join(
            self.test_dir, '..', 'sonic-cfggen')]
        self.sample_yang_file = os.path.join(self.test_dir,
                                             'test_yang_data.json')

    def run_script(self, arg, check_stderr=False):
        print('\n    Running sonic-cfggen ', arg)
        if check_stderr:
            output = subprocess.check_output(self.script_file + arg,
                                             stderr=subprocess.STDOUT)
        else:
            output = subprocess.check_output(self.script_file + arg)

        if utils.PY3x:
            output = output.decode()

        linecount = output.strip().count('\n')
        if linecount <= 0:
            print('    Output: ' + output.strip())
        else:
            print('    Output: ({0} lines, {1} bytes)'.format(
                linecount + 1, len(output)))
        return output

    def run_diff(self, file1, file2):
        _, output = getstatusoutput_noshell(['diff', '-u', file1, file2])
        return output

    def run_script_with_yang_arg(self, arg, check_stderr=False):
        args = ["-Y", self.sample_yang_file] + arg
        return self.run_script(arg=args, check_stderr=check_stderr)

    def test_print_data(self):
        arg = ["--print-data"]
        output = self.run_script_with_yang_arg(arg)
        assert len(output.strip()) > 0


    def test_jinja_expression(self, expected_router_type='LeafRouter'):
        arg = ["-v", "DEVICE_METADATA[\'localhost\'][\'type\']"]
        output = self.run_script_with_yang_arg(arg)
        assert output.strip() == expected_router_type

    def test_hwsku(self):
        arg = ["-v", "DEVICE_METADATA[\'localhost\'][\'hwsku\']"]
        output = self.run_script_with_yang_arg(arg)
        assert output.strip() == "Force10-S6000"

    def test_device_metadata(self):
        arg = ["--var-json", "DEVICE_METADATA"]
        output = json.loads(self.run_script_with_yang_arg(arg))
        assert (output['localhost'] ==  {\
            'bgp_asn': '65100',
            'docker_routing_config_mode': 'unified',
            'hostname': 'sonic',
            'hwsku': 'Force10-S6000',
            'platform': 'x86_64-kvm_x86_64-r0',
            'type': 'LeafRouter',
            'yang_config_validation': 'disable'
        })


    def test_port_table(self):
        arg = ["--var-json", "PORT"]
        output = json.loads(self.run_script_with_yang_arg(arg))
        assert(output == \
            {'Ethernet0': {'admin_status': 'up', 'alias': 'eth0', 'description': 'Ethernet0', 'fec': 'rs', 'lanes': '65, 66', 'mtu': '9100', 'pfc_asym': 'on', 'speed': '40000'},
            'Ethernet4': {'admin_status': 'up', 'alias': 'eth4', 'description': 'Ethernet4', 'fec': 'rs', 'lanes': '67, 68', 'mtu': '9100', 'pfc_asym': 'on', 'speed': '40000'},
            'Ethernet8': {'admin_status': 'up', 'alias': 'eth8', 'description': 'Ethernet8', 'fec': 'rs', 'lanes': '69, 70', 'mtu': '9100', 'pfc_asym': 'on', 'speed': '40000'},
            'Ethernet12': {'admin_status': 'up', 'alias': 'eth12', 'description': 'Ethernet12', 'fec': 'rs', 'lanes': '71, 72', 'mtu': '9100', 'pfc_asym': 'on', 'speed': '40000'},
            'Ethernet16': {'admin_status': 'up', 'alias': 'eth16', 'description': 'Ethernet16', 'fec': 'rs', 'lanes': '73, 74', 'mtu': '9100', 'pfc_asym': 'on', 'speed': '40000'},
            'Ethernet20': {'admin_status': 'up', 'alias': 'eth20', 'description': 'Ethernet20', 'fec': 'rs', 'lanes': '75,76', 'mtu': '9100', 'pfc_asym': 'on', 'speed': '40000'},
            'Ethernet24': {'admin_status': 'up', 'alias': 'eth24', 'description': 'Ethernet24', 'fec': 'rs', 'lanes': '77,78', 'mtu': '9100', 'pfc_asym': 'on', 'speed': '40000'},
            'Ethernet28': {'admin_status': 'up', 'alias': 'eth28', 'description': 'Ethernet28', 'fec': 'rs', 'lanes': '79,80', 'mtu': '9100', 'pfc_asym': 'on', 'speed': '40000'}
            })

    def test_portchannel_table(self):
        arg = ["--var-json", "PORTCHANNEL"]
        output = json.loads(self.run_script_with_yang_arg(arg))
        assert(output == \
            {'PortChannel1001': {'admin_status': 'up',
                      'lacp_key': 'auto',
                      'min_links': '1',
                      'mtu': '9100'},
            'PortChannel1002': {'admin_status': 'up',
                      'lacp_key': 'auto',
                      'min_links': '1',
                      'mtu': '9100'}})

    def test_portchannel_member_table(self):
        arg = ["--var-json", "PORTCHANNEL_MEMBER"]
        output = json.loads(self.run_script_with_yang_arg(arg))
        assert(output ==\
            {   "PortChannel1001|Ethernet0": {},
                "PortChannel1001|Ethernet4": {},
                "PortChannel1002|Ethernet16": {},
                "PortChannel1002|Ethernet20": {}
            })

    def test_interface_table(self):
        arg = ["--var-json", "INTERFACE"]
        output = json.loads(self.run_script_with_yang_arg(arg))
        assert(output =={\
            "Ethernet8": {},
            "Ethernet12": {},
            "Ethernet8|10.0.0.8/31": {
                "family": "IPv4",
                "scope": "global"
            },
            "Ethernet8|fc::01/126": {
                "family": "IPv6",
                "scope": "global"
            },
            "Ethernet12|10.0.0.12/31": {
                "family": "IPv4",
                "scope": "global"
            },
            "Ethernet12|fd::01/126": {
                "family": "IPv6",
                "scope": "global"
            }
        })

    def test_portchannel_interface_table(self):
        arg = ["--var-json", "PORTCHANNEL_INTERFACE"]
        output = json.loads(self.run_script_with_yang_arg(arg))
        assert(output =={\
            "PortChannel1001|10.0.0.1/31": {},
            "PortChannel1002|10.0.0.60/31": {}
            })

    def test_loopback_table(self):
        arg = ["--var-json", "LOOPBACK_INTERFACE"]
        output = json.loads(self.run_script_with_yang_arg(arg))
        assert(output == {\
            "Loopback0": {},
            "Loopback0|aa::01/64": {
                "family": "IPv6",
                "scope": "global"
            },
            "Loopback0|10.1.0.32/32": {
                "family": "IPv4",
                "scope": "global"
            }
        })

    def test_acl_table(self):
        arg = ["--var-json", "ACL_TABLE"]
        output = json.loads(self.run_script_with_yang_arg(arg))
        assert(output == {\
            'DATAACL':   {'policy_desc': 'DATAACL',    'ports': ['PortChannel1001','PortChannel1002'], 'stage': 'ingress', 'type': 'L3'},
            'EVERFLOW':  {'policy_desc': 'EVERFLOW',   'ports': ['PortChannel1001','PortChannel1002'], 'stage': 'ingress', 'type': 'MIRROR'},
            'EVERFLOWV6': {'policy_desc': 'EVERFLOWV6', 'ports': ['PortChannel1001','PortChannel1002'], 'stage': 'ingress', 'type': 'MIRRORV6'},
            'SNMP_ACL':  {'policy_desc': 'SNMP_ACL',    'services': ['SNMP'],        'stage': 'ingress', 'type': 'CTRLPLANE'},
            'SSH_ONLY':  {'policy_desc': 'SSH_ONLY',    'services': ['SSH'],         'stage': 'ingress', 'type': 'CTRLPLANE'}})

    def test_acl_rule(self):
        arg = ["--var-json", "ACL_RULE"]
        output = json.loads(self.run_script_with_yang_arg(arg))
        assert(output == {\
            "DATAACL|Rule1": {
                "DST_IP": "192.168.1.1/24",
                "SRC_IP": "10.10.1.1/16",
                "IP_TYPE": "IPV4",
                "PACKET_ACTION": "DROP",
                "PRIORITY": "100"
            },
            "EVERFLOW|Rule2": {
                "DST_IP": "192.169.10.1/32",
                "SRC_IP": "10.10.1.1/16",
                "IP_TYPE": "IPV4",
                "PRIORITY": "101"
            }
        })

    def test_vlan_table(self):
        arg = ["--var-json", "VLAN"]
        output = json.loads(self.run_script_with_yang_arg(arg))
        assert(output == {\
            "Vlan100": {
                "admin_status": "up",
                "description": "server_vlan",
                "dhcp_servers": [
                    "192.169.1.1",
                    "198.169.1.1",
                    "199.169.1.2"
                ],
                "mtu": "9100",
                "vlanid": "100"
            }
        })

    def test_vlan_interface(self):
        arg = ["--var-json", "VLAN_INTERFACE"]
        output = json.loads(self.run_script_with_yang_arg(arg))
        assert(output == {\
            "Vlan100": {},
            "Vlan100|bb::01/64": {
                "family": "IPv6",
                "scope": "global"
            },
            "Vlan100|10.35.61.1/24": {
                "family": "IPv4",
                "scope": "global"
            }
        })

    def test_vlan_member(self):
        arg = ["--var-json", "VLAN_MEMBER"]
        output = json.loads(self.run_script_with_yang_arg(arg))
        assert(output == {\
            "Vlan100|Ethernet24": {
                "tagging_mode": "untagged"
            },
            "Vlan100|Ethernet28": {
                "tagging_mode": "untagged"
            }
        })

    def test_vlan_crm(self):
        arg = ["--var-json", "CRM"]
        output = json.loads(self.run_script_with_yang_arg(arg))
        assert(output == {\
            "Config": {
                "acl_counter_high_threshold": "85",
                "acl_counter_low_threshold": "25",
                "acl_counter_threshold_type": "used",
                "ipv4_route_high_threshold": "90",
                "ipv4_route_low_threshold": "10",
                "ipv4_route_threshold_type": "used",
                "ipv6_route_high_threshold": "90",
                "ipv6_route_low_threshold": "10",
                "ipv6_route_threshold_type": "used",
                "ipv4_neighbor_high_threshold": "85",
                "ipv4_neighbor_low_threshold": "25",
                "ipv4_neighbor_threshold_type": "used",
                "ipv6_neighbor_high_threshold": "90",
                "ipv6_neighbor_low_threshold": "10",
                "ipv6_neighbor_threshold_type": "used"
            }
        })

    def test_fabric_monitor_data_table(self):
        arg = ["--var-json", "FABRIC_MONITOR"]
        output = json.loads(self.run_script_with_yang_arg(arg))
        assert(output == {\
            "FABRIC_MONITOR_DATA": {
                "monErrThreshCrcCells": "1",
                "monErrThreshRxCells": "61035156",
                "monPollThreshIsolation": "1",
                "monPollThreshRecovery": "8",
                "monCapacityThreshWarn": "10",
                "monState": "enable"
            }
        })

    def test_fabric_port_table(self):
        arg = ["--var-json", "FABRIC_PORT"]
        output = json.loads(self.run_script_with_yang_arg(arg))
        assert(output == {\
            "Fabric0": {
                "alias": "Fabric0",
                "isolateStatus": "False",
                "lanes": "0",
                "forceUnisolateStatus": "0"
            }
        })
