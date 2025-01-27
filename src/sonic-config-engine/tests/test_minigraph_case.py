import json
import os
import subprocess
import ipaddress
import tests.common_utils as utils
import minigraph

from unittest import TestCase

TOR_ROUTER = 'ToRRouter'
BACKEND_TOR_ROUTER = 'BackEndToRRouter'
BMC_MGMT_TOR_ROUTER = 'BmcMgmtToRRouter'

class TestCfgGenCaseInsensitive(TestCase):

    def setUp(self):
        self.yang = utils.YangWrapper()
        self.test_dir = os.path.dirname(os.path.realpath(__file__))
        self.script_file = [utils.PYTHON_INTERPRETTER, os.path.join(self.test_dir, '..', 'sonic-cfggen')]
        self.sample_graph = os.path.join(self.test_dir, 'simple-sample-graph-case.xml')
        self.sample_simple_graph = os.path.join(self.test_dir, 'simple-sample-graph.xml')
        self.sample_resource_graph = os.path.join(self.test_dir, 'sample-graph-resource-type.xml')
        self.sample_subintf_graph = os.path.join(self.test_dir, 'sample-graph-subintf.xml')
        self.sample_simple_device_desc = os.path.join(self.test_dir, 'simple-sample-device-desc.xml')
        self.sample_simple_device_desc_ipv6_only = os.path.join(self.test_dir, 'simple-sample-device-desc-ipv6-only.xml')
        self.port_config = os.path.join(self.test_dir, 't0-sample-port-config.ini')

    def run_script(self, argument, check_stderr=False):
        print('\n    Running sonic-cfggen ' + ' '.join(argument))
        self.assertTrue(self.yang.validate(argument))

        if check_stderr:
            output = subprocess.check_output(self.script_file + argument, stderr=subprocess.STDOUT)
        else:
            output = subprocess.check_output(self.script_file + argument)

        if utils.PY3x:
            output = output.decode()

        linecount = output.strip().count('\n')
        if linecount <= 0:
            print('    Output: ' + output.strip())
        else:
            print('    Output: ({0} lines, {1} bytes)'.format(linecount + 1, len(output)))
        return output

    def test_dummy_run(self):
        argument = []
        output = self.run_script(argument)
        self.assertEqual(output, '')

    def test_minigraph_sku(self):
        argument = ['-v', "DEVICE_METADATA[\'localhost\'][\'hwsku\']", '-m', self.sample_graph, '-p', self.port_config]
        output = self.run_script(argument)
        self.assertEqual(output.strip(), 'Force10-S6000')

    def test_print_data(self):
        argument = ['-m', self.sample_graph, '-p', self.port_config, '--print-data']
        output = self.run_script(argument)
        self.assertTrue(len(output.strip()) > 0)

    def test_jinja_expression(self):
        argument = ['-m', self.sample_graph, '-p', self.port_config, '-v', "DEVICE_METADATA[\'localhost\'][\'type\']"]
        output = self.run_script(argument)
        self.assertEqual(output.strip(), 'ToRRouter')

    def test_minigraph_subtype(self):
        argument = ['-m', self.sample_graph, '-p', self.port_config, '-v', "DEVICE_METADATA[\'localhost\'][\'subtype\']"]
        output = self.run_script(argument)
        self.assertEqual(output.strip(), 'DualToR')

    def test_minigraph_peer_switch_hostname(self):
        argument = ['-m', self.sample_graph, '-p', self.port_config, '-v', "DEVICE_METADATA[\'localhost\'][\'peer_switch\']"]
        output = self.run_script(argument)
        self.assertEqual(output.strip(), 'switch2-t0')

    def test_additional_json_data(self):
        argument = ['-a', '{"key1":"value1"}', '-v', 'key1']
        output = self.run_script(argument)
        self.assertEqual(output.strip(), 'value1')

    def test_read_yaml(self):
        argument = ['-v', 'yml_item', '-y', os.path.join(self.test_dir, 'test.yml')]
        output = self.run_script(argument)
        self.assertEqual(output.strip(), '[\'value1\', \'value2\']')

    def test_render_template(self):
        argument = ['-y', os.path.join(self.test_dir, 'test.yml'), '-t', os.path.join(self.test_dir, 'test.j2')]
        output = self.run_script(argument)
        self.assertEqual(output.strip(), 'value1\nvalue2')

#     everflow portion is not used
#     def test_minigraph_everflow(self):
#         argument = '-m "' + self.sample_graph + '" -p "' + self.port_config + '" -v MIRROR_SESSION'
#         output = self.run_script(argument)
#         self.assertEqual(output.strip(), "{'everflow0': {'src_ip': '10.1.0.32', 'dst_ip': '10.0.100.1'}}")

    def test_minigraph_interfaces(self):
        argument = ['-m', self.sample_graph, '-p', self.port_config, '-v', 'INTERFACE.keys()|list']
        output = self.run_script(argument)
        self.assertEqual(output.strip(), "[('Ethernet0', '10.0.0.58/31'), 'Ethernet0', ('Ethernet0', 'FC00::75/126')]")

    def test_minigraph_vlans(self):
        argument = ['-m', self.sample_graph, '-p', self.port_config, '-v', 'VLAN']
        output = self.run_script(argument)

        expected = {
                   'Vlan1000': {
                       'alias': 'ab1',
                       'dhcp_servers': ['192.0.0.1', '192.0.0.2'],
                       'dhcpv6_servers': ['fc02:2000::1', 'fc02:2000::2'],
                       'vlanid': '1000',
                       'mac': '00:aa:bb:cc:dd:ee',
                       },
                   'Vlan2000': {
                       'alias': 'ab2',
                       'dhcp_servers': ['192.0.0.1'],
                       'dhcpv6_servers': ['fc02:2000::3', 'fc02:2000::4'],
                       'vlanid': '2000'
                       }
                   }
        self.assertEqual(
            utils.to_dict(output.strip()),
            expected
        )

    def test_minigraph_vlan_members(self):
        argument = ['-m', self.sample_graph, '-p', self.port_config, '-v', 'VLAN_MEMBER']
        output = self.run_script(argument)
        expected = {
                       'Vlan1000|Ethernet8': {'tagging_mode': 'untagged'},
                       'Vlan2000|PortChannel01': {'tagging_mode': 'untagged'}
                   }
        self.assertEqual(
                utils.to_dict(output.strip()),
                expected
        )

    def test_minigraph_vlan_interfaces_keys(self):
        argument = ['-m', self.sample_graph, '-p', self.port_config, '-v', "VLAN_INTERFACE.keys()|list"]
        output = self.run_script(argument)
        expected_list_dict = {
            'list': ['Vlan1000', 'Vlan1000|192.168.0.1/27', 'Vlan1000|192.168.1.1/27']
        }
        self.assertEqual(utils.liststr_to_dict(output.strip()), expected_list_dict)

    def test_minigraph_vlan_interfaces(self):
        argument = ['-m', self.sample_graph, '-p', self.port_config, '-v', "VLAN_INTERFACE"]
        output = self.run_script(argument)
        expected_table = {
            'Vlan1000|192.168.0.1/27': {},
            'Vlan1000': {
                'proxy_arp': 'enabled',
                'grat_arp': 'enabled'
            },
            'Vlan1000|192.168.1.1/27': {
                'secondary': 'true'
            }
        }
        self.assertEqual(utils.to_dict(output.strip()), expected_table)

    def test_minigraph_portchannels(self):
        argument = ['-m', self.sample_graph, '-p', self.port_config, '-v', 'PORTCHANNEL']
        output = self.run_script(argument)
        self.assertEqual(
            utils.to_dict(output.strip()),
            utils.to_dict("{'PortChannel01': {'admin_status': 'up', 'min_links': '1', 'mtu': '9100', 'tpid': '0x8100', 'lacp_key': 'auto'}}")
        )

    def test_minigraph_console_mgmt_feature(self):
        argument = ['-m', self.sample_graph, '-p', self.port_config, '-v', 'CONSOLE_SWITCH']
        output = self.run_script(argument)
        self.assertEqual(
            utils.to_dict(output.strip()),
            utils.to_dict("{'console_mgmt': {'enabled': 'no'}}"))

    def test_minigraph_console_port(self):
        argument = ['-m', self.sample_graph, '-p', self.port_config, '-v', 'CONSOLE_PORT']
        output = self.run_script(argument)
        self.assertEqual(
            utils.to_dict(output.strip()),
            utils.to_dict("{'1': {'baud_rate': '9600', 'remote_device': 'managed_device', 'flow_control': 1}}"))

    def test_minigraph_dhcp_server_feature(self):
        argument = ['-m', self.sample_graph, '-p', self.port_config, '-v', "DEVICE_METADATA[\'localhost\'][\'dhcp_server\']"]
        output = self.run_script(argument)
        self.assertEqual(output.strip(), '')

        try:
            # For DHCP server enabled device type
            output = subprocess.check_output(["sed", "-i", 's/%s/%s/g' % (TOR_ROUTER, BMC_MGMT_TOR_ROUTER), self.sample_graph])
            output = self.run_script(argument)
            self.assertEqual(output.strip(), 'enabled')
        finally:
            output = subprocess.check_output(["sed", "-i", 's/%s/%s/g' % (BMC_MGMT_TOR_ROUTER, TOR_ROUTER), self.sample_graph])

    def test_minigraph_deployment_id(self):
        argument = ['-m', self.sample_graph, '-p', self.port_config, '-v', "DEVICE_METADATA[\'localhost\'][\'deployment_id\']"]
        output = self.run_script(argument)
        self.assertEqual(output.strip(), "1")

    def test_minigraph_rack_mgmt_map(self):
        argument = ['-m', self.sample_graph, '-p', self.port_config, '-v', "DEVICE_METADATA[\'localhost\'][\'rack_mgmt_map\']"]
        output = self.run_script(argument)
        self.assertEqual(output.strip(), "dummy_value")

    def test_minigraph_cluster(self):
        argument = ['-m', self.sample_graph, '-p', self.port_config, '-v', "DEVICE_METADATA[\'localhost\'][\'cluster\']"]
        output = self.run_script(argument)
        self.assertEqual(output.strip(), "AAA00PrdStr00")

    def test_minigraph_neighbor_metadata(self):
        argument = ['-m', self.sample_graph, '-p', self.port_config, '-v', "DEVICE_NEIGHBOR_METADATA"]

        expected_table = {
            'switch2-t0': {
                'cluster': 'DB5PrdApp11',
                'lo_addr': '25.1.1.10/32',
                'mgmt_addr': '10.7.0.196/26',
                'hwsku': 'Force10-S6000',
                'type': 'ToRRouter'
            },
            'server2': {
                'lo_addr_v6': 'fe80::0002/128',
                'lo_addr': '10.10.10.2/32',
                'mgmt_addr': '10.0.0.2/32',
                'hwsku': 'server-sku',
                'type': 'Server'
            },
            'server1': {
                'lo_addr_v6': 'fe80::0001/80',
                'lo_addr': '10.10.10.1/32',
                'mgmt_addr': '10.0.0.1/32',
                'hwsku': 'server-sku',
                'type': 'Server'
            },
            'switch-01t1': {
                'cluster': 'DB5PrdApp11',
                'lo_addr': '10.1.0.186/32',
                'deployment_id': '2',
                'hwsku': 'Force10-S6000',
                'type': 'LeafRouter',
                'mgmt_addr': '10.7.0.196/26'
            },
            'server1-SC': {
                'lo_addr_v6': '::/0',
                'mgmt_addr': '0.0.0.0/0',
                'hwsku': 'smartcable-sku',
                'lo_addr': '0.0.0.0/0',
                'type': 'SmartCable',
                'mgmt_addr_v6': '::/0',
            }
        }
        output = self.run_script(argument)
        self.maxDiff = None
        self.assertEqual(
            utils.to_dict(output.strip()),
            expected_table
        )

#     everflow portion is not used
#     def test_metadata_everflow(self):
#         argument = '-m "' + self.sample_graph + '" -p "' + self.port_config + '" -v "MIRROR_SESSION"'
#         output = self.run_script(argument)
#         self.assertEqual(output.strip(), "{'everflow0': {'src_ip': '10.1.0.32', 'dst_ip': '10.0.100.1'}}")

    def test_metadata_tacacs(self):
        argument = ['-m', self.sample_graph, '-p', self.port_config, '-v', "TACPLUS_SERVER"]
        output = self.run_script(argument)
        self.assertEqual(output.strip(), "{'10.0.10.7': {'priority': '1', 'tcp_port': '49'}, '10.0.10.8': {'priority': '1', 'tcp_port': '49'}}")

    def test_metadata_kube(self):
        argument = ['-m', self.sample_graph, '-p', self.port_config, '-v', "KUBERNETES_MASTER[\'SERVER\']"]
        output = self.run_script(argument)
        self.assertEqual(json.loads(output.strip().replace("'", "\"")),
                json.loads('{"ip": "10.10.10.10", "disable": "True"}'))

    def test_minigraph_mgmt_port(self):
        argument = ['-m', self.sample_graph, '-p', self.port_config, '-v', "MGMT_PORT"]
        output = self.run_script(argument)
        self.assertEqual(output.strip(), "{'eth0': {'alias': 'eth0', 'admin_status': 'up', 'speed': '1000'}}")

    def test_metadata_ntp(self):
        argument = ['-m', self.sample_graph, '-p', self.port_config, '-v', "NTP_SERVER"]
        output = self.run_script(argument)
        self.assertEqual(output.strip(), "{'10.0.10.1': {'iburst': 'on'}, '10.0.10.2': {'iburst': 'on'}}")

    def test_minigraph_vnet(self):
        argument = ['-m', self.sample_graph, '-p', self.port_config, '-v', "VNET"]
        output = self.run_script(argument)
        self.assertEqual(output.strip(), "")

    def test_minigraph_vxlan(self):
        argument = ['-m', self.sample_graph, '-p', self.port_config, '-v', "VXLAN_TUNNEL"]
        output = self.run_script(argument)
        self.assertEqual(output.strip(), "")

    def test_minigraph_bgp_mon(self):
        argument = ['-m', self.sample_graph, '-p', self.port_config, '-v', "BGP_MONITORS"]
        output = self.run_script(argument)
        self.assertEqual(output.strip(), "{}")

    def test_minigraph_peer_switch(self):
        argument = ['-m', self.sample_graph, '-p', self.port_config, '-v', "PEER_SWITCH"]
        expected_table = {
            'switch2-t0': {
                'address_ipv4': "25.1.1.10"
            }
        }

        output = self.run_script(argument)
        self.assertEqual(
            utils.to_dict(output.strip()),
            expected_table
        )

    def test_mux_cable_parsing(self):
        result = minigraph.parse_xml(self.sample_graph, port_config_file=self.port_config)

        expected_mux_cable_ports = ["Ethernet4", "Ethernet8"]
        port_table = result['PORT']
        for port_name, port in port_table.items():
            if port_name in expected_mux_cable_ports:
                self.assertTrue(port["mux_cable"])
            else:
                self.assertTrue("mux_cable" not in port)

    def test_minigraph_storage_device(self):
        argument = ['-m', self.sample_graph, '-p', self.port_config, '-v', "DEVICE_METADATA[\'localhost\'][\'storage_device\']"]
        output = self.run_script(argument)
        self.assertEqual(output.strip(), "true")

    def test_minigraph_storage_backend_no_resource_type(self):
        self.verify_storage_device_set(self.sample_simple_graph)

    def test_minigraph_storage_backend_resource_type(self):
        self.verify_storage_device_set(self.sample_resource_graph)

    def test_minigraph_storage_backend_subintf(self):
        self.verify_storage_device_set(self.sample_subintf_graph)

    def verify_storage_device_set(self, graph_file, check_stderr=False):
        try:
            print('\n    Change device type to %s' % (BACKEND_TOR_ROUTER))
            if check_stderr:
                output = subprocess.check_output(["sed", "-i", 's/%s/%s/g' % (TOR_ROUTER, BACKEND_TOR_ROUTER), graph_file], stderr=subprocess.STDOUT)
            else:
                output = subprocess.check_output(["sed", "-i", 's/%s/%s/g' % (TOR_ROUTER, BACKEND_TOR_ROUTER), graph_file])

            argument = ['-m', graph_file, '-p', self.port_config, '-v', "DEVICE_METADATA[\'localhost\'][\'storage_device\']"]
            output = self.run_script(argument)
            self.assertEqual(output.strip(), "true")

        finally:
            print('\n    Change device type back to %s' % (TOR_ROUTER))
            if check_stderr:
                output = subprocess.check_output(["sed", "-i", 's/%s/%s/g' % (BACKEND_TOR_ROUTER, TOR_ROUTER), graph_file], stderr=subprocess.STDOUT)
            else:
                output = subprocess.check_output(["sed", "-i", 's/%s/%s/g' % (BACKEND_TOR_ROUTER, TOR_ROUTER), graph_file])

    def test_minigraph_tunnel_table(self):
        argument = ['-m', self.sample_graph, '-p', self.port_config, '-v', "TUNNEL"]
        expected_tunnel = {
            "MuxTunnel0": {
                "tunnel_type": "IPINIP",
                "dst_ip": "10.1.0.32",
                "dscp_mode": "uniform",
                "encap_ecn_mode": "standard",
                "ecn_mode": "copy_from_outer",
                "ttl_mode": "pipe"
            }
        }

        output = self.run_script(argument)
        self.assertEqual(
            utils.to_dict(output.strip()),
            expected_tunnel
        )

        # Validate tunnel config is as before when tunnel_qos_remap = disabled
        sample_graph_disabled_remap = os.path.join(self.test_dir, 'simple-sample-graph-case-remap-disabled.xml')
        argument = ['-m', sample_graph_disabled_remap, '-p', self.port_config, '-v', "TUNNEL"]

        output = self.run_script(argument)
        self.assertEqual(
            utils.to_dict(output.strip()),
            expected_tunnel
        )

        # Validate extra config is generated when tunnel_qos_remap = enabled
        sample_graph_enabled_remap = os.path.join(self.test_dir, 'simple-sample-graph-case-remap-enabled.xml')
        argument = ['-m', sample_graph_enabled_remap, '-p', self.port_config, '-v', "TUNNEL"]
        expected_tunnel = {
            "MuxTunnel0": {
                "tunnel_type": "IPINIP",
                "src_ip": "25.1.1.10",
                "dst_ip": "10.1.0.32",
                "dscp_mode": "pipe",
                "encap_ecn_mode": "standard",
                "ecn_mode": "copy_from_outer",
                "ttl_mode": "pipe",
                "decap_dscp_to_tc_map": "AZURE_TUNNEL",
                "decap_tc_to_pg_map": "AZURE_TUNNEL",
                "encap_tc_to_dscp_map": "AZURE_TUNNEL",
                "encap_tc_to_queue_map": "AZURE_TUNNEL"
            }
        }

        output = self.run_script(argument)
        self.assertEqual(
            utils.to_dict(output.strip()),
            expected_tunnel
        )

        # Validate extra config for mux tunnel is generated automatically when tunnel_qos_remap = enabled
        sample_graph_enabled_remap = os.path.join(self.test_dir, 'simple-sample-graph-case-remap-enabled-no-tunnel-attributes.xml')
        argument = ['-m', sample_graph_enabled_remap, '-p', self.port_config, '-v', "TUNNEL"]
        output = self.run_script(argument)
        self.assertEqual(
            utils.to_dict(output.strip()),
            expected_tunnel
        )

    def test_minigraph_mux_cable_table(self):
        argument = ['-m', self.sample_graph, '-p', self.port_config, '-v', "MUX_CABLE"]
        expected_table = {
            'Ethernet4': {
                'state': 'auto',
                'server_ipv4': '10.10.10.1/32',
                'server_ipv6': 'fe80::1/128'
            },
            'Ethernet8': {
                'state': 'auto',
                'server_ipv4': '10.10.10.2/32',
                'server_ipv6': 'fe80::2/128',
                'soc_ipv4': '10.10.10.3/32',
                'soc_ipv6': 'fe80::3/128',
                'cable_type': 'active-active'
            }
        }

        output = self.run_script(argument)
        self.assertEqual(
            utils.to_dict(output.strip()),
            expected_table
        )

    def test_dhcp_table(self):
        argument = ['-m', self.sample_graph, '-p', self.port_config, '-v', "DHCP_RELAY"]
        expected = {
                   'Vlan1000': {
                       'dhcpv6_servers': [
                           "fc02:2000::1",
                           "fc02:2000::2"
                       ]
                    },
                    'Vlan2000': {
                       'dhcpv6_servers': [
                           "fc02:2000::3",
                           "fc02:2000::4"
                       ]
                    }
        }
        output = self.run_script(argument)
        self.assertEqual(
            utils.to_dict(output.strip()),
            expected
        )

    def test_minigraph_mirror_dscp(self):
        result = minigraph.parse_xml(self.sample_graph, port_config_file=self.port_config)
        self.assertTrue('EVERFLOW_DSCP' in result['ACL_TABLE'])
        everflow_dscp_entry = result['ACL_TABLE']['EVERFLOW_DSCP']

        self.assertEqual(everflow_dscp_entry['type'], 'MIRROR_DSCP')
        self.assertEqual(everflow_dscp_entry['stage'], 'ingress')
        expected_ports = ['PortChannel01', 'Ethernet12', 'Ethernet8', 'Ethernet0']
        self.assertEqual(
            everflow_dscp_entry['ports'].sort(),
            expected_ports.sort()
        )

    def test_minigraph_acl_attach_to_ports(self):
        """
        The test case is to verify ACL table can be bound to both port names and alias
        """
        sample_graph = os.path.join(self.test_dir,'simple-sample-graph-case-acl-test.xml')
        port_config_duplicated_name_alias = os.path.join(self.test_dir, 't0-sample-port-config-duplicated-name-alias.ini')
        result = minigraph.parse_xml(sample_graph, port_config_file=port_config_duplicated_name_alias)
        # TC1: All ports are portchannels or port names
        expected_dataacl_ports = ['PortChannel01','Ethernet20','Ethernet24']
        self.assertEqual(sorted(result['ACL_TABLE']['DATAACL_PORT_NAME']['ports']), sorted(expected_dataacl_ports))
        # TC2: All ports are portchanels or port alias
        expected_dataacl_ports = ['PortChannel01','Ethernet4','Ethernet8']
        self.assertEqual(sorted(result['ACL_TABLE']['DATAACL_PORT_ALIAS']['ports']), sorted(expected_dataacl_ports))
        # TC3: Duplicated values in port names and alias, but all fall in port names
        expected_dataacl_ports = ['PortChannel01','Ethernet0','Ethernet1','Ethernet2','Ethernet3']
        self.assertEqual(sorted(result['ACL_TABLE']['DATAACL_MIXED_NAME_ALIAS_1']['ports']), sorted(expected_dataacl_ports))
        # TC4: Duplicated values in port names and alias, but all fall in port alias
        expected_dataacl_ports = ['PortChannel01','Ethernet0','Ethernet1','Ethernet4','Ethernet8']
        self.assertEqual(sorted(result['ACL_TABLE']['DATAACL_MIXED_NAME_ALIAS_2']['ports']), sorted(expected_dataacl_ports))
        # TC5: Same count in port names and alias, port alias is preferred
        expected_dataacl_ports = ['Ethernet0']
        self.assertEqual(sorted(result['ACL_TABLE']['DATAACL_MIXED_NAME_ALIAS_3']['ports']), sorted(expected_dataacl_ports))

    def test_minigraph_acl_type_bmcdata(self):
        expected_acl_type_bmcdata = {
            "ACTIONS": ["PACKET_ACTION", "COUNTER"],
            "BIND_POINTS": ["PORT"],
            "MATCHES": ["SRC_IP", "DST_IP", "ETHER_TYPE", "IP_TYPE", "IP_PROTOCOL", "IN_PORTS", "L4_SRC_PORT", "L4_DST_PORT", "L4_SRC_PORT_RANGE", "L4_DST_PORT_RANGE"],
        }
        expected_acl_type_bmcdatav6 = {
            "ACTIONS": ["PACKET_ACTION", "COUNTER"],
            "BIND_POINTS": ["PORT"],
            "MATCHES": ["SRC_IPV6", "DST_IPV6", "ETHER_TYPE", "IP_TYPE", "IP_PROTOCOL", "IN_PORTS", "L4_SRC_PORT", "L4_DST_PORT", "L4_SRC_PORT_RANGE", "L4_DST_PORT_RANGE", "ICMPV6_TYPE", "ICMPV6_CODE", "TCP_FLAGS"],
        }
        expected_acl_table_bmc_acl_northbound =  {
            'policy_desc': 'BMC_ACL_NORTHBOUND',
            'ports': ['Ethernet0', 'Ethernet1'],
            'stage': 'ingress',
            'type': 'BMCDATA',
        }
        expected_acl_table_bmc_acl_northbound_v6 = {
            'policy_desc': 'BMC_ACL_NORTHBOUND_V6',
            'ports': ['Ethernet0', 'Ethernet1'],
            'stage': 'ingress',
            'type': 'BMCDATAV6',
        }
        # TC1: Minigraph contains acl table type BmcData
        sample_mx_graph = os.path.join(self.test_dir,'simple-sample-graph-mx.xml')
        result = minigraph.parse_xml(sample_mx_graph, port_config_file=self.port_config)
        self.assertIn('ACL_TABLE_TYPE', result)
        self.assertIn('BMCDATA', result['ACL_TABLE_TYPE'])
        self.assertIn('BMCDATAV6', result['ACL_TABLE_TYPE'])
        self.assertDictEqual(result['ACL_TABLE_TYPE']['BMCDATA'], expected_acl_type_bmcdata)
        self.assertDictEqual(result['ACL_TABLE_TYPE']['BMCDATAV6'], expected_acl_type_bmcdatav6)
        self.assertDictEqual(result['ACL_TABLE']['BMC_ACL_NORTHBOUND'], expected_acl_table_bmc_acl_northbound)
        self.assertDictEqual(result['ACL_TABLE']['BMC_ACL_NORTHBOUND_V6'], expected_acl_table_bmc_acl_northbound_v6)
        # TC2: Minigraph doesn't contain acl table type BmcData
        result = minigraph.parse_xml(self.sample_graph, port_config_file=self.port_config)
        self.assertNotIn('ACL_TABLE_TYPE', result)

    def test_parse_device_desc_xml_mgmt_interface(self):
        # Regular device_desc.xml with both IPv4 and IPv6 mgmt address
        result = minigraph.parse_device_desc_xml(self.sample_simple_device_desc)
        mgmt_intf = result['MGMT_INTERFACE']
        self.assertEqual(len(mgmt_intf.keys()), 2)
        self.assertTrue(('eth0', '10.0.0.100/24') in mgmt_intf.keys())
        self.assertTrue(('eth0', 'FC00:1::32/64') in mgmt_intf.keys())
        self.assertTrue(ipaddress.ip_address(u'10.0.0.1') == mgmt_intf[('eth0', '10.0.0.100/24')]['gwaddr'])
        self.assertTrue(ipaddress.ip_address(u'fc00:1::1') == mgmt_intf[('eth0', 'FC00:1::32/64')]['gwaddr'])

        # Special device_desc.xml with IPv6 mgmt address only
        result = minigraph.parse_device_desc_xml(self.sample_simple_device_desc_ipv6_only)
        mgmt_intf = result['MGMT_INTERFACE']
        self.assertEqual(len(mgmt_intf.keys()), 1)
        self.assertTrue(('eth0', 'FC00:1::32/64') in mgmt_intf.keys())
        self.assertTrue(ipaddress.ip_address(u'fc00:1::1') == mgmt_intf[('eth0', 'FC00:1::32/64')]['gwaddr'])

    def test_mgmt_device_disable_counters(self):
        expected_mgmt_disabled_counters = ["BUFFER_POOL_WATERMARK", "PFCWD", "PG_DROP", "PG_WATERMARK", "PORT_BUFFER_DROP", "QUEUE", "QUEUE_WATERMARK"]
        expected_mgmt_enabled_counters = ["ACL", "PORT", "RIF"]
        # TC1: For M0 and Mx minigraph, counters are configured as expected
        mgmt_graphs = ['simple-sample-graph-mx.xml', 'simple-sample-graph-m0.xml']
        for graph in mgmt_graphs:
            graph_path = os.path.join(self.test_dir, graph)
            result = minigraph.parse_xml(graph_path, port_config_file=self.port_config)
            self.assertIn('FLEX_COUNTER_TABLE', result)
            for counter in expected_mgmt_disabled_counters:
                self.assertIn(counter, result['FLEX_COUNTER_TABLE'])
                self.assertDictEqual(result['FLEX_COUNTER_TABLE'][counter], {'FLEX_COUNTER_STATUS': 'disable'})
            for counter in expected_mgmt_enabled_counters:
                if counter in result['FLEX_COUNTER_TABLE']:
                    self.assertDictEqual(result['FLEX_COUNTER_TABLE'][counter], {'FLEX_COUNTER_STATUS': 'enable'})
        # TC2: For other minigraph, result should not contain FLEX_COUNTER_TABLE
        result = minigraph.parse_xml(self.sample_graph, port_config_file=self.port_config)
        self.assertNotIn('FLEX_COUNTER_TABLE', result)
