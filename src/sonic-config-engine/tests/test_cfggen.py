import json
import subprocess
import os
import tests.common_utils as utils

from unittest import TestCase

TOR_ROUTER = 'ToRRouter'
BACKEND_TOR_ROUTER = 'BackEndToRRouter'
LEAF_ROUTER = 'LeafRouter'
BACKEND_LEAF_ROUTER = 'BackEndLeafRouter'

class TestCfgGen(TestCase):

    def setUp(self):
        self.yang = utils.YangWrapper()
        self.test_dir = os.path.dirname(os.path.realpath(__file__))
        self.script_file = [utils.PYTHON_INTERPRETTER, os.path.join(self.test_dir, '..', 'sonic-cfggen')]
        self.sample_graph = os.path.join(self.test_dir, 'sample_graph.xml')
        self.sample_graph_t0 = os.path.join(self.test_dir, 't0-sample-graph.xml')
        self.sample_graph_simple = os.path.join(self.test_dir, 'simple-sample-graph.xml')
        self.sample_graph_simple_case = os.path.join(self.test_dir, 'simple-sample-graph-case.xml')
        self.sample_graph_metadata = os.path.join(self.test_dir, 'simple-sample-graph-metadata.xml')
        self.sample_graph_pc_test = os.path.join(self.test_dir, 'pc-test-graph.xml')
        self.sample_graph_bgp_speaker = os.path.join(self.test_dir, 't0-sample-bgp-speaker.xml')
        self.sample_graph_deployment_id = os.path.join(self.test_dir, 't0-sample-deployment-id.xml')
        self.sample_graph_voq = os.path.join(self.test_dir, 'sample-voq-graph.xml')
        self.sample_device_desc = os.path.join(self.test_dir, 'device.xml')
        self.port_config = os.path.join(self.test_dir, 't0-sample-port-config.ini')
        self.port_config_autoneg = os.path.join(self.test_dir, 't0-sample-autoneg-port-config.ini')
        self.mlnx_port_config = os.path.join(self.test_dir, 'mellanox-sample-port-config.ini')
        self.output_file = os.path.join(self.test_dir, 'output')
        self.output2_file = os.path.join(self.test_dir, 'output2')
        self.ecmp_graph = os.path.join(self.test_dir, 'fg-ecmp-sample-minigraph.xml')
        self.sample_resource_graph = os.path.join(self.test_dir, 'sample-graph-resource-type.xml')
        self.sample_subintf_graph = os.path.join(self.test_dir, 'sample-graph-subintf.xml')
        self.voq_port_config = os.path.join(self.test_dir, 'voq-sample-port-config.ini')
        self.packet_chassis_graph0 = os.path.join(self.test_dir, 'minigraph-str2-temp-2-lc04.xml')
        self.packet_chassis_graph = os.path.join(self.test_dir, 'sample-chassis-packet-lc-graph.xml')
        self.packet_chassis_port_ini = os.path.join(self.test_dir, 'sample-chassis-packet-lc-port-config.ini')
        self.macsec_profile = os.path.join(self.test_dir, 'macsec_profile.json')
        self.sample_backend_graph = os.path.join(self.test_dir, 'sample-graph-storage-backend.xml')
        self.voq_port_config_400g = os.path.join(self.test_dir, 'voq-sample-400g-port-config.ini')
        self.voq_sample_masic_graph = os.path.join(self.test_dir, 'voq-sample-masic-graph.xml')
        self.sample_cisco_port_config_400g = os.path.join(self.test_dir, 'sample-cisco-8101-t1-port-config.ini')
        self.sample_cisco_100_graph = os.path.join(self.test_dir, 'sample-cisco-8101-t1-100-minigraph.xml')
        self.sample_cisco_400_graph = os.path.join(self.test_dir, 'sample-cisco-8101-t1-400-minigraph.xml')
        self.sample_cisco_8111_port_config = os.path.join(self.test_dir, 'sample-cisco-8111-port-config.ini')
        self.sample_cisco_8111_graph = os.path.join(self.test_dir, 'sample-cisco-8111-100-minigraph.xml')
        # To ensure that mock config_db data is used for unit-test cases
        os.environ["CFGGEN_UNIT_TESTING"] = "2"

    def tearDown(self):
        os.environ["CFGGEN_UNIT_TESTING"] = ""
        try:
            os.remove(self.output_file)
            os.remove(self.output2_file)
        except OSError:
            pass

    def run_script(self, argument, check_stderr=False, verbose=False):
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
            if verbose == True:
                print('    Output: ' + output.strip())
        return output

    def test_dummy_run(self):
        argument = []
        output = self.run_script(argument)
        self.assertEqual(output, '')

    def test_device_desc(self):
        argument = ['-v', "DEVICE_METADATA[\'localhost\'][\'hwsku\']", "-M", self.sample_device_desc]
        output = self.run_script(argument)
        self.assertEqual(output.strip(), 'ACS-MSN2700')

    def test_device_desc_mgmt_ip(self):
        argument = ['-v', "(MGMT_INTERFACE.keys()|list)[0]", '-M', self.sample_device_desc]
        output = self.run_script(argument)
        self.assertEqual(output.strip(), "('eth0', '10.0.1.5/28')")

    def test_minigraph_hostname(self):
        argument = ['-v', "DEVICE_METADATA[\'localhost\'][\'hostname\']", '-m', self.sample_graph, "-p", self.port_config]
        output = self.run_script(argument)
        self.assertEqual(output.strip(), 'OCPSCH01040DDLF')

    def test_minigraph_sku(self):
        argument = ['-v', "DEVICE_METADATA[\'localhost\'][\'hwsku\']", '-m', self.sample_graph, '-p', self.port_config]
        output = self.run_script(argument)
        self.assertEqual(output.strip(), 'Force10-Z9100')

    def test_minigraph_region(self):
        argument = ['-v', "DEVICE_METADATA[\'localhost\'][\'region\']", '-m', self.sample_graph_metadata, '-p', self.port_config]
        output = self.run_script(argument)
        self.assertEqual(output.strip(), 'usfoo')

    def test_minigraph_cloudtype(self):
        argument = ['-v', "DEVICE_METADATA[\'localhost\'][\'cloudtype\']", '-m', self.sample_graph_metadata, '-p', self.port_config]
        output = self.run_script(argument)
        self.assertEqual(output.strip(), 'Public')

    def test_minigraph_resourcetype(self):
        argument = ['-v', "DEVICE_METADATA[\'localhost\'][\'resource_type\']", '-m', self.sample_graph_metadata, '-p', self.port_config]
        output = self.run_script(argument)
        self.assertEqual(output.strip(), 'resource_type_x')

    def test_minigraph_downstream_subrole(self):
        argument = ['-v', "DEVICE_METADATA[\'localhost\'][\'downstream_subrole\']", '-m', self.sample_graph_metadata, '-p', self.port_config]
        output = self.run_script(argument)
        self.assertEqual(output.strip(), 'downstream_subrole_y')

    def test_print_data(self):
        argument = ['-m', self.sample_graph, '-p', self.port_config, '--print-data']
        output = self.run_script(argument)
        self.assertTrue(len(output.strip()) > 0)

    def test_jinja_expression(self, graph=None, port_config=None, expected_router_type='LeafRouter'):
        if graph is None:
            graph = self.sample_graph
        if port_config is None:
            port_config = self.port_config
        argument = ['-m', graph, '-p', port_config, '-v', "DEVICE_METADATA[\'localhost\'][\'type\']"]
        output = self.run_script(argument)
        self.assertEqual(output.strip(), expected_router_type)

    def test_additional_json_data(self):
        argument = ['-a', '{"key1":"value1"}', '-v', 'key1']
        output = self.run_script(argument)
        self.assertEqual(output.strip(), 'value1')

    def test_additional_json_data_level1_key(self):
        argument = ['-a', '{"k1":{"k11":"v11","k12":"v12"}, "k2":{"k22":"v22"}}', '--var-json', 'k1']
        output = self.run_script(argument)
        self.assertEqual(utils.to_dict(output.strip()), utils.to_dict('{\n    "k11": "v11", \n    "k12": "v12"\n}'))

    def test_additional_json_data_level2_key(self):
        argument = ['-a', '{"k1":{"k11":"v11","k12":"v12"},"k2":{"k22":"v22"}}', '--var-json', 'k1', '-K', 'k11']
        output = self.run_script(argument)
        self.assertEqual(utils.to_dict(output.strip()), utils.to_dict('{\n    "k11": "v11"\n}'))

    def test_var_json_data(self, **kwargs):
        graph_file = kwargs.get('graph_file', self.sample_graph_simple)
        tag_mode = kwargs.get('tag_mode', 'untagged')
        argument = ['-m', graph_file, '-p', self.port_config, '--var-json', 'VLAN_MEMBER']
        output = self.run_script(argument)
        if tag_mode == "tagged":
            self.assertEqual(
                utils.to_dict(output.strip()),
                utils.to_dict(
                    '{\n    "Vlan1000|Ethernet8": {\n        "tagging_mode": "tagged"\n    },'
                    ' \n    "Vlan1001|Ethernet8": {\n        "tagging_mode": "tagged"\n    },'
                    ' \n    "Vlan2000|Ethernet12": {\n        "tagging_mode": "tagged"\n    },'
                    ' \n    "Vlan2001|Ethernet12": {\n        "tagging_mode": "tagged"\n    },'
                    ' \n    "Vlan2020|Ethernet12": {\n        "tagging_mode": "tagged"\n    }\n}'
                )
            )
        else:
            self.assertEqual(
                utils.to_dict(output.strip()),
                utils.to_dict(
                    '{\n    "Vlan1000|Ethernet8": {\n        "tagging_mode": "tagged"\n    },'
                    ' \n    "Vlan1001|Ethernet8": {\n        "tagging_mode": "tagged"\n    },'
                    ' \n    "Vlan2000|Ethernet12": {\n        "tagging_mode": "tagged"\n    },'
                    ' \n    "Vlan2001|Ethernet12": {\n        "tagging_mode": "untagged"\n    },'
                    ' \n    "Vlan2020|Ethernet12": {\n        "tagging_mode": "tagged"\n    }\n}'
                )
            )

    def test_read_yaml(self):
        argument = ['-v', 'yml_item', '-y', os.path.join(self.test_dir, 'test.yml')]
        output = self.run_script(argument)
        self.assertEqual(output.strip(), '[\'value1\', \'value2\']')

    def test_render_template(self):
        argument = ['-y', os.path.join(self.test_dir, 'test.yml'), '-t', os.path.join(self.test_dir, 'test.j2')]
        output = self.run_script(argument)
        self.assertEqual(output.strip(), 'value1\nvalue2')

    def test_template_batch_mode(self):
        argument = ['-y', os.path.join(self.test_dir, 'test.yml')]
        argument += ['-a', '{"key1":"value"}']
        argument += ['-t', os.path.join(self.test_dir, 'test.j2') + ',' + self.output_file]
        argument += ['-t', os.path.join(self.test_dir, 'test2.j2') + ',' + self.output2_file]
        output = self.run_script(argument)
        assert(os.path.exists(self.output_file))
        assert(os.path.exists(self.output2_file))
        with open(self.output_file) as tf:
            self.assertEqual(tf.read().strip(), 'value1\nvalue2')
        with open(self.output2_file) as tf:
            self.assertEqual(tf.read().strip(), 'value')

    def test_template_json_batch_mode(self):
        data = {"key1_1":"value1_1", "key1_2":"value1_2", "key2_1":"value2_1", "key2_2":"value2_2"}
        argument = ["-a", '{0}'.format(repr(data).replace('\'', '"'))]
        argument += ['-t', os.path.join(self.test_dir, 'sample-template-1.json.j2') + ",config-db"]
        argument += ['-t', os.path.join(self.test_dir, 'sample-template-2.json.j2') + ",config-db"]
        argument += ['--print-data']
        output = self.run_script(argument)
        output_data = json.loads(output)
        for key, value in data.items():
            self.assertEqual(output_data[key.replace("key", "jk")], value)

    # FIXME: This test depends heavily on the ordering of the interfaces and
    # it is not at all intuitive what that ordering should be. Could make it
    # more robust by adding better parsing logic.
    def test_minigraph_acl(self):
        argument = ['-m', self.sample_graph_t0, '-p', self.port_config, '-v', 'ACL_TABLE']
        output = self.run_script(argument, True, True)
        self.assertEqual(
            utils.to_dict(output.strip().replace("Warning: Ignoring Control Plane ACL NTP_ACL without type\n", '')),
            utils.to_dict(
                "{'NTP_ACL': {'services': ['NTP'], 'type': 'CTRLPLANE', 'policy_desc': 'NTP_ACL', 'stage': 'ingress'}, "
                "'EVERFLOW': {'stage': 'ingress', 'type': 'MIRROR', 'ports': ['PortChannel01', 'PortChannel02', 'PortChannel03', 'PortChannel04', 'Ethernet4', 'Ethernet100'], 'policy_desc': 'EVERFLOW'}, "
                "'EVERFLOW_EGRESS': {'stage': 'egress', 'type': 'MIRROR', 'ports': ['PortChannel01', 'PortChannel02', 'PortChannel03', 'PortChannel04', 'Ethernet4', 'Ethernet100'], 'policy_desc': 'EVERFLOW_EGRESS'}, "
                "'ROUTER_PROTECT': {'services': ['SSH', 'SNMP'], 'type': 'CTRLPLANE', 'policy_desc': 'ROUTER_PROTECT', 'stage': 'ingress'}, "
                "'DATAACLINGRESS': {'stage': 'ingress', 'type': 'L3', 'ports': ['PortChannel01', 'PortChannel02', 'PortChannel03', 'PortChannel04'], 'policy_desc': 'DATAACLINGRESS'}, "
                "'SNMP_ACL': {'services': ['SNMP'], 'type': 'CTRLPLANE', 'policy_desc': 'SNMP_ACL', 'stage': 'ingress'}, "
                "'SSH_ACL': {'services': ['SSH'], 'type': 'CTRLPLANE', 'policy_desc': 'SSH_ACL', 'stage': 'ingress'}, "
                "'DATAACLEGRESS': {'stage': 'egress', 'type': 'L3', 'ports': ['PortChannel01', 'PortChannel02', 'Ethernet100', 'PortChannel03'], 'policy_desc': 'DATAACLEGRESS'}, "
                "'EVERFLOWV6': {'stage': 'ingress', 'type': 'MIRRORV6', 'ports': ['PortChannel01', 'PortChannel02', 'PortChannel03', 'PortChannel04', 'Ethernet4', 'Ethernet100'], 'policy_desc': 'EVERFLOWV6'}}"
            )
        )

#     everflow portion is not used
#     def test_minigraph_everflow(self):
#         argument = '-m "' + self.sample_graph_t0 + '" -p "' + self.port_config + '" -v MIRROR_SESSION'
#         output = self.run_script(argument)
#         self.assertEqual(output.strip(), "{'everflow0': {'src_ip': '10.1.0.32', 'dst_ip': '2.2.2.2'}}")

    def test_minigraph_mgmt_ports(self):
        argument = ['-m', self.sample_graph, '-p', self.port_config, '-v','MGMT_PORT']
        output = self.run_script(argument)
        self.assertEqual(
            utils.to_dict(output.strip()),
            utils.to_dict("{'eth0': {'alias': 'Management0', 'admin_status': 'up'}}")
        )

    def test_minigraph_interfaces(self):
        argument = ['-m', self.sample_graph_simple, '-p', self.port_config, '-v', "INTERFACE.keys()|list"]
        output = self.run_script(argument)
        self.assertEqual(output.strip(), "[('Ethernet0', '10.0.0.58/31'), 'Ethernet0', ('Ethernet0', 'FC00::75/126')]")

    def test_minigraph_vlans(self, **kwargs):
        graph_file = kwargs.get('graph_file', self.sample_graph_simple)
        argument = ['-m', graph_file, '-p', self.port_config, '-v', 'VLAN']
        output = self.run_script(argument)
        self.assertEqual(
            utils.to_dict(output.strip()),
            utils.to_dict(
                "{'Vlan1000': {'alias': 'ab1', 'dhcp_servers': ['192.0.0.1', '192.0.0.2'], 'vlanid': '1000'}, "
                "'Vlan1001': {'alias': 'ab4', 'dhcp_servers': ['192.0.0.1', '192.0.0.2'], 'vlanid': '1001'},"
                "'Vlan2001': {'alias': 'ab3', 'dhcp_servers': ['192.0.0.1', '192.0.0.2'], 'vlanid': '2001'},"
                "'Vlan2000': {'alias': 'ab2', 'dhcp_servers': ['192.0.0.1', '192.0.0.2'], 'vlanid': '2000'},"
                "'Vlan2020': {'alias': 'kk1', 'dhcp_servers': ['192.0.0.1', '192.0.0.2'], 'vlanid': '2020'}}"
            )
        )

    def test_minigraph_vlan_members(self, **kwargs):
        graph_file = kwargs.get('graph_file', self.sample_graph_simple)
        tag_mode = kwargs.get('tag_mode', 'untagged')
        argument = ['-m', graph_file, '-p', self.port_config, '-v', 'VLAN_MEMBER']
        output = self.run_script(argument)
        if tag_mode == "tagged":
            self.assertEqual(
                utils.to_dict(output.strip()),
                utils.to_dict(
                    "{('Vlan2000', 'Ethernet12'): {'tagging_mode': 'tagged'}, "
                    "('Vlan1001', 'Ethernet8'): {'tagging_mode': 'tagged'}, "
                    "('Vlan1000', 'Ethernet8'): {'tagging_mode': 'tagged'}, "
                    "('Vlan2020', 'Ethernet12'): {'tagging_mode': 'tagged'}, "
                    "('Vlan2001', 'Ethernet12'): {'tagging_mode': 'tagged'}}"
                )
            )
        else:
            self.assertEqual(
                utils.to_dict(output.strip()),
                utils.to_dict(
                    "{('Vlan2000', 'Ethernet12'): {'tagging_mode': 'tagged'}, "
                    "('Vlan1000', 'Ethernet8'): {'tagging_mode': 'tagged'}, "
                    "('Vlan1001', 'Ethernet8'): {'tagging_mode': 'tagged'}, "
                    "('Vlan2020', 'Ethernet12'): {'tagging_mode': 'tagged'}, "
                    "('Vlan2001', 'Ethernet12'): {'tagging_mode': 'untagged'}}"
                )
            )

    def test_minigraph_vlan_interfaces(self, **kwargs):
        graph_file = kwargs.get('graph_file', self.sample_graph_simple)
        argument = ['-m', graph_file, '-p', self.port_config, '-v', "VLAN_INTERFACE.keys()|list"]
        output = self.run_script(argument)
        self.assertEqual(output.strip(), "[('Vlan1000', '192.168.0.1/27'), 'Vlan1000']")

    def test_minigraph_ecmp_fg_nhg(self):
        argument = ['-m', self.ecmp_graph, '-p', self.mlnx_port_config, '-v', 'FG_NHG']
        output = self.run_script(argument)
        print(output.strip())
        self.assertEqual(utils.to_dict(output.strip()),
                         utils.to_dict(
                            "{'fgnhg_v4': {'match_mode': 'nexthop-based', 'bucket_size': 120}, "
                            "'fgnhg_v6': {'match_mode': 'nexthop-based', 'bucket_size': 120}}"
                        ))

    def test_minigraph_ecmp_members(self):
        argument = ['-m', self.ecmp_graph, '-p', self.mlnx_port_config, '-v', "FG_NHG_MEMBER.keys()|list|sort"]
        output = self.run_script(argument)
        self.assertEqual(output.strip(), "['200.200.200.1', '200.200.200.10', '200.200.200.2', '200.200.200.3', '200.200.200.4', '200.200.200.5',"
                                         " '200.200.200.6', '200.200.200.7', '200.200.200.8', '200.200.200.9', '200:200:200:200::1', '200:200:200:200::10',"
                                         " '200:200:200:200::2', '200:200:200:200::3', '200:200:200:200::4', '200:200:200:200::5', '200:200:200:200::6',"
                                         " '200:200:200:200::7', '200:200:200:200::8', '200:200:200:200::9']")

    def test_minigraph_ecmp_neighbors(self):
        argument = ['-m', self.ecmp_graph, '-p', self.mlnx_port_config, '-v', "NEIGH.keys()|list|sort"]
        output = self.run_script(argument)
        self.assertEqual(output.strip(), "['Vlan31|200.200.200.1', 'Vlan31|200.200.200.10', 'Vlan31|200.200.200.2', 'Vlan31|200.200.200.3',"
                                         " 'Vlan31|200.200.200.4', 'Vlan31|200.200.200.5', 'Vlan31|200.200.200.6', 'Vlan31|200.200.200.7',"
                                         " 'Vlan31|200.200.200.8', 'Vlan31|200.200.200.9', 'Vlan31|200:200:200:200::1', 'Vlan31|200:200:200:200::10',"
                                         " 'Vlan31|200:200:200:200::2', 'Vlan31|200:200:200:200::3', 'Vlan31|200:200:200:200::4', 'Vlan31|200:200:200:200::5', "
                                         "'Vlan31|200:200:200:200::6', 'Vlan31|200:200:200:200::7', 'Vlan31|200:200:200:200::8', 'Vlan31|200:200:200:200::9']")

    def test_minigraph_portchannels(self, **kwargs):
        graph_file = kwargs.get('graph_file', self.sample_graph_simple)
        argument = ['-m', graph_file, '-p', self.port_config, '-v', 'PORTCHANNEL']
        output = self.run_script(argument)
        self.assertEqual(
            utils.to_dict(output.strip()),
            utils.to_dict("{'PortChannel1': {'admin_status': 'up', 'min_links': '1', 'mtu': '9100', 'tpid': '0x8100', 'lacp_key': 'auto'}}")
        )

    def test_minigraph_portchannel_with_more_member(self):
        argument = ['-m', self.sample_graph_pc_test, '-p', self.port_config, '-v', 'PORTCHANNEL']
        output = self.run_script(argument)
        self.assertEqual(
            utils.to_dict(output.strip()),
            utils.to_dict("{'PortChannel01': {'admin_status': 'up', 'min_links': '3', 'mtu': '9100', 'tpid': '0x8100', 'lacp_key': 'auto'}}"))

    def test_minigraph_portchannel_members(self):
        argument = ['-m', self.sample_graph_pc_test, '-p', self.port_config, '-v', "PORTCHANNEL_MEMBER.keys()|list"]
        output = self.run_script(argument)
        self.assertEqual(
            utils.liststr_to_dict(output.strip()),
            utils.liststr_to_dict("[('PortChannel01', 'Ethernet120'), ('PortChannel01', 'Ethernet116'), ('PortChannel01', 'Ethernet124'), ('PortChannel01', 'Ethernet112')]")
        )

    def test_minigraph_portchannel_interfaces(self):
        argument = ['-m', self.sample_graph_simple, '-p', self.port_config, '-v', "PORTCHANNEL_INTERFACE.keys()|list"]
        output = self.run_script(argument)
        self.assertEqual(
            utils.liststr_to_dict(output.strip()),
            utils.liststr_to_dict("['PortChannel1', ('PortChannel1', '10.0.0.56/31'), ('PortChannel1', 'FC00::71/126')]")
        )

    def test_minigraph_neighbors(self):
        argument = ['-m', self.sample_graph_t0, '-p', self.port_config, '-v', "DEVICE_NEIGHBOR[\'Ethernet124\']"]
        output = self.run_script(argument)
        self.assertEqual(
            utils.to_dict(output.strip()),
            utils.to_dict("{'name': 'ARISTA04T1', 'port': 'Ethernet1/1'}")
        )

    # FIXME: This test depends heavily on the ordering of the interfaces and
    # it is not at all intuitive what that ordering should be. Could make it
    # more robust by adding better parsing logic.
    def test_minigraph_extra_neighbors(self):
        argument = ['-m', self.sample_graph_t0, '-p', self.port_config, '-v', 'DEVICE_NEIGHBOR']
        output = self.run_script(argument)
        self.assertEqual(
            utils.to_dict(output.strip()),
            utils.to_dict(
                "{'Ethernet124': {'name': 'ARISTA04T1', 'port': 'Ethernet1/1'}, "
                "'Ethernet120': {'name': 'ARISTA03T1', 'port': 'Ethernet1/1'}, "
                "'Ethernet4': {'name': 'Servers0', 'port': 'eth0'}, "
                "'Ethernet116': {'name': 'ARISTA02T1', 'port': 'Ethernet1/1'}, "
                "'Ethernet100': {'name': 'Servers100', 'port': 'eth0'}, "
                "'Ethernet112': {'name': 'ARISTA01T1', 'port': 'Ethernet1/1'}}")
        )

    def test_minigraph_port_description(self):
        argument = ['-m', self.sample_graph_t0, '-p', self.port_config, '-v', "PORT[\'Ethernet124\']"]
        output = self.run_script(argument)
        self.assertEqual(
            utils.to_dict(output.strip()),
            utils.to_dict("{'lanes': '101,102,103,104', 'fec': 'rs', 'pfc_asym': 'off', 'mtu': '9100', 'tpid': '0x8100', 'alias': 'fortyGigE0/124', 'admin_status': 'up', 'speed': '100000', 'description': 'ARISTA04T1:Ethernet1/1'}")
        )

    def test_minigraph_port_fec_disabled(self):
        # Test for FECDisabled
        argument = ['-m', self.sample_graph_t0, '-p', self.port_config, '-v', "PORT[\'Ethernet4\']"]
        output = self.run_script(argument)
        self.assertEqual(
            utils.to_dict(output.strip()),
            utils.to_dict("{'lanes': '25,26,27,28', 'description': 'Servers0:eth0', 'pfc_asym': 'off', 'mtu': '9100', 'tpid': '0x8100', 'alias': 'fortyGigE0/4', 'admin_status': 'up', 'speed': '100000', 'autoneg': 'on', 'fec': 'none'}")
        )

    def test_minigraph_port_autonegotiation(self):
        # Test with a port_config.ini file which doesn't have an 'autoneg' column
        argument = ['-m', self.sample_graph_t0, '-p', self.port_config, '-v', "PORT"]
        output = self.run_script(argument)
        self.assertEqual(
            utils.to_dict(output.strip()),
            utils.to_dict(
                "{'Ethernet0': {'alias': 'fortyGigE0/0', 'pfc_asym': 'off', 'lanes': '29,30,31,32', 'description': 'fortyGigE0/0', 'mtu': '9100', 'tpid': '0x8100', 'speed': '40000'}, "
                "'Ethernet4': {'lanes': '25,26,27,28', 'description': 'Servers0:eth0', 'pfc_asym': 'off', 'mtu': '9100', 'tpid': '0x8100', 'alias': 'fortyGigE0/4', 'admin_status': 'up', 'autoneg': 'on', 'speed': '100000', 'fec': 'none'}, "
                "'Ethernet8': {'lanes': '37,38,39,40', 'description': 'fortyGigE0/8', 'pfc_asym': 'off', 'mtu': '9100', 'tpid': '0x8100', 'alias': 'fortyGigE0/8', 'admin_status': 'up', 'autoneg': 'off', 'fec': 'none', 'speed': '40000'}, "
                "'Ethernet12': {'lanes': '33,34,35,36', 'description': 'fortyGigE0/12', 'pfc_asym': 'off', 'mtu': '9100', 'tpid': '0x8100', 'alias': 'fortyGigE0/12', 'admin_status': 'up', 'speed': '40000'}, "
                "'Ethernet16': {'lanes': '41,42,43,44', 'description': 'fortyGigE0/16', 'pfc_asym': 'off', 'mtu': '9100', 'tpid': '0x8100', 'alias': 'fortyGigE0/16', 'admin_status': 'up', 'speed': '40000'}, "
                "'Ethernet20': {'lanes': '45,46,47,48', 'description': 'fortyGigE0/20', 'pfc_asym': 'off', 'mtu': '9100', 'tpid': '0x8100', 'alias': 'fortyGigE0/20', 'admin_status': 'up', 'speed': '40000'}, "
                "'Ethernet24': {'lanes': '5,6,7,8', 'description': 'fortyGigE0/24', 'pfc_asym': 'off', 'mtu': '9100', 'tpid': '0x8100', 'alias': 'fortyGigE0/24', 'admin_status': 'up', 'speed': '40000'}, "
                "'Ethernet28': {'lanes': '1,2,3,4', 'description': 'fortyGigE0/28', 'pfc_asym': 'off', 'mtu': '9100', 'tpid': '0x8100', 'alias': 'fortyGigE0/28', 'admin_status': 'up', 'speed': '40000'}, "
                "'Ethernet32': {'lanes': '9,10,11,12', 'description': 'fortyGigE0/32', 'pfc_asym': 'off', 'mtu': '9100', 'tpid': '0x8100', 'alias': 'fortyGigE0/32', 'admin_status': 'up', 'speed': '40000'}, "
                "'Ethernet36': {'lanes': '13,14,15,16', 'description': 'fortyGigE0/36', 'pfc_asym': 'off', 'mtu': '9100', 'tpid': '0x8100', 'alias': 'fortyGigE0/36', 'admin_status': 'up', 'speed': '40000'}, "
                "'Ethernet40': {'lanes': '21,22,23,24', 'description': 'fortyGigE0/40', 'pfc_asym': 'off', 'mtu': '9100', 'tpid': '0x8100', 'alias': 'fortyGigE0/40', 'admin_status': 'up', 'speed': '40000'}, "
                "'Ethernet44': {'lanes': '17,18,19,20', 'description': 'fortyGigE0/44', 'pfc_asym': 'off', 'mtu': '9100', 'tpid': '0x8100', 'alias': 'fortyGigE0/44', 'admin_status': 'up', 'speed': '40000'}, "
                "'Ethernet48': {'lanes': '49,50,51,52', 'description': 'fortyGigE0/48', 'pfc_asym': 'off', 'mtu': '9100', 'tpid': '0x8100', 'alias': 'fortyGigE0/48', 'admin_status': 'up', 'speed': '40000'}, "
                "'Ethernet52': {'lanes': '53,54,55,56', 'description': 'fortyGigE0/52', 'pfc_asym': 'off', 'mtu': '9100', 'tpid': '0x8100', 'alias': 'fortyGigE0/52', 'admin_status': 'up', 'speed': '40000'}, "
                "'Ethernet56': {'lanes': '61,62,63,64', 'description': 'fortyGigE0/56', 'pfc_asym': 'off', 'mtu': '9100', 'tpid': '0x8100', 'alias': 'fortyGigE0/56', 'admin_status': 'up', 'speed': '40000'}, "
                "'Ethernet60': {'lanes': '57,58,59,60', 'description': 'fortyGigE0/60', 'pfc_asym': 'off', 'mtu': '9100', 'tpid': '0x8100', 'alias': 'fortyGigE0/60', 'admin_status': 'up', 'speed': '40000'}, "
                "'Ethernet64': {'lanes': '65,66,67,68', 'description': 'fortyGigE0/64', 'pfc_asym': 'off', 'mtu': '9100', 'tpid': '0x8100', 'alias': 'fortyGigE0/64', 'admin_status': 'up', 'speed': '40000'}, "
                "'Ethernet68': {'lanes': '69,70,71,72', 'description': 'fortyGigE0/68', 'pfc_asym': 'off', 'mtu': '9100', 'tpid': '0x8100', 'alias': 'fortyGigE0/68', 'admin_status': 'up', 'speed': '40000'}, "
                "'Ethernet72': {'lanes': '77,78,79,80', 'description': 'fortyGigE0/72', 'pfc_asym': 'off', 'mtu': '9100', 'tpid': '0x8100', 'alias': 'fortyGigE0/72', 'admin_status': 'up', 'speed': '40000'}, "
                "'Ethernet76': {'lanes': '73,74,75,76', 'description': 'fortyGigE0/76', 'pfc_asym': 'off', 'mtu': '9100', 'tpid': '0x8100', 'alias': 'fortyGigE0/76', 'admin_status': 'up', 'speed': '40000'}, "
                "'Ethernet80': {'lanes': '105,106,107,108', 'description': 'fortyGigE0/80', 'pfc_asym': 'off', 'mtu': '9100', 'tpid': '0x8100', 'alias': 'fortyGigE0/80', 'admin_status': 'up', 'speed': '40000'}, "
                "'Ethernet84': {'lanes': '109,110,111,112', 'description': 'fortyGigE0/84', 'pfc_asym': 'off', 'mtu': '9100', 'tpid': '0x8100', 'alias': 'fortyGigE0/84', 'admin_status': 'up', 'speed': '40000'}, "
                "'Ethernet88': {'lanes': '117,118,119,120', 'description': 'fortyGigE0/88', 'pfc_asym': 'off', 'mtu': '9100', 'tpid': '0x8100', 'alias': 'fortyGigE0/88', 'admin_status': 'up', 'speed': '40000'}, "
                "'Ethernet92': {'lanes': '113,114,115,116', 'description': 'fortyGigE0/92', 'pfc_asym': 'off', 'mtu': '9100', 'tpid': '0x8100', 'alias': 'fortyGigE0/92', 'admin_status': 'up', 'speed': '40000'}, "
                "'Ethernet96': {'lanes': '121,122,123,124', 'description': 'fortyGigE0/96', 'pfc_asym': 'off', 'mtu': '9100', 'tpid': '0x8100', 'alias': 'fortyGigE0/96', 'admin_status': 'up', 'speed': '40000'}, "
                "'Ethernet100': {'lanes': '125,126,127,128', 'description': 'Servers100:eth0', 'pfc_asym': 'off', 'mtu': '9100', 'tpid': '0x8100', 'alias': 'fortyGigE0/100', 'admin_status': 'up', 'speed': '40000'}, "
                "'Ethernet104': {'alias': 'fortyGigE0/104', 'pfc_asym': 'off', 'lanes': '85,86,87,88', 'description': 'fortyGigE0/104', 'mtu': '9100', 'tpid': '0x8100', 'speed': '40000'}, "
                "'Ethernet108': {'alias': 'fortyGigE0/108', 'pfc_asym': 'off', 'lanes': '81,82,83,84', 'description': 'fortyGigE0/108', 'mtu': '9100', 'tpid': '0x8100', 'speed': '40000'}, "
                "'Ethernet112': {'lanes': '89,90,91,92', 'description': 'ARISTA01T1:Ethernet1/1', 'pfc_asym': 'off', 'mtu': '9100', 'tpid': '0x8100', 'alias': 'fortyGigE0/112', 'admin_status': 'up', 'speed': '40000'}, "
                "'Ethernet116': {'lanes': '93,94,95,96', 'description': 'ARISTA02T1:Ethernet1/1', 'pfc_asym': 'off', 'mtu': '9100', 'tpid': '0x8100', 'alias': 'fortyGigE0/116', 'admin_status': 'up', 'speed': '40000'}, "
                "'Ethernet120': {'lanes': '97,98,99,100', 'description': 'ARISTA03T1:Ethernet1/1', 'pfc_asym': 'off', 'mtu': '9100', 'tpid': '0x8100', 'alias': 'fortyGigE0/120', 'admin_status': 'up', 'speed': '40000'}, "
                "'Ethernet124': {'lanes': '101,102,103,104', 'fec': 'rs', 'pfc_asym': 'off', 'mtu': '9100', 'tpid': '0x8100', 'alias': 'fortyGigE0/124', 'admin_status': 'up', 'speed': '100000', 'description': 'ARISTA04T1:Ethernet1/1'}}"
            )
        )

        # Test with a port_config.ini file which has an 'autoneg' column
        argument = ['-m', self.sample_graph_t0, '-p', self.port_config_autoneg, '-v', "PORT"]
        output = self.run_script(argument)
        self.assertEqual(
            utils.to_dict(output.strip()),
            utils.to_dict(
                "{'Ethernet0': {'lanes': '29,30,31,32', 'description': 'fortyGigE0/0', 'mtu': '9100', 'tpid': '0x8100', 'alias': 'fortyGigE0/0', 'pfc_asym': 'off', 'autoneg': 'off', 'speed': '40000'}, "
                "'Ethernet4': {'lanes': '25,26,27,28', 'description': 'Servers0:eth0', 'pfc_asym': 'off', 'mtu': '9100', 'tpid': '0x8100', 'alias': 'fortyGigE0/4', 'admin_status': 'up', 'autoneg': 'on', 'speed': '100000', 'fec': 'none'}, "
                "'Ethernet8': {'lanes': '37,38,39,40', 'description': 'fortyGigE0/8', 'pfc_asym': 'off', 'mtu': '9100', 'tpid': '0x8100', 'alias': 'fortyGigE0/8', 'admin_status': 'up', 'autoneg': 'off', 'fec': 'none', 'speed': '40000'}, "
                "'Ethernet12': {'lanes': '33,34,35,36', 'description': 'fortyGigE0/12', 'pfc_asym': 'off', 'mtu': '9100', 'tpid': '0x8100', 'alias': 'fortyGigE0/12', 'admin_status': 'up', 'autoneg': 'off', 'speed': '40000'}, "
                "'Ethernet16': {'lanes': '41,42,43,44', 'description': 'fortyGigE0/16', 'pfc_asym': 'off', 'mtu': '9100', 'tpid': '0x8100', 'alias': 'fortyGigE0/16', 'admin_status': 'up', 'autoneg': 'off', 'speed': '40000'}, "
                "'Ethernet20': {'lanes': '45,46,47,48', 'description': 'fortyGigE0/20', 'pfc_asym': 'off', 'mtu': '9100', 'tpid': '0x8100', 'alias': 'fortyGigE0/20', 'admin_status': 'up', 'autoneg': 'off', 'speed': '40000'}, "
                "'Ethernet24': {'lanes': '5,6,7,8', 'description': 'fortyGigE0/24', 'pfc_asym': 'off', 'mtu': '9100', 'tpid': '0x8100', 'alias': 'fortyGigE0/24', 'admin_status': 'up', 'autoneg': 'off', 'speed': '40000'}, "
                "'Ethernet28': {'lanes': '1,2,3,4', 'description': 'fortyGigE0/28', 'pfc_asym': 'off', 'mtu': '9100', 'tpid': '0x8100', 'alias': 'fortyGigE0/28', 'admin_status': 'up', 'autoneg': 'off', 'speed': '40000'}, "
                "'Ethernet32': {'lanes': '9,10,11,12', 'description': 'fortyGigE0/32', 'pfc_asym': 'off', 'mtu': '9100', 'tpid': '0x8100', 'alias': 'fortyGigE0/32', 'admin_status': 'up', 'autoneg': 'off', 'speed': '40000'}, "
                "'Ethernet36': {'lanes': '13,14,15,16', 'description': 'fortyGigE0/36', 'pfc_asym': 'off', 'mtu': '9100', 'tpid': '0x8100', 'alias': 'fortyGigE0/36', 'admin_status': 'up', 'autoneg': 'off', 'speed': '40000'}, "
                "'Ethernet40': {'lanes': '21,22,23,24', 'description': 'fortyGigE0/40', 'pfc_asym': 'off', 'mtu': '9100', 'tpid': '0x8100', 'alias': 'fortyGigE0/40', 'admin_status': 'up', 'autoneg': 'off', 'speed': '40000'}, "
                "'Ethernet44': {'lanes': '17,18,19,20', 'description': 'fortyGigE0/44', 'pfc_asym': 'off', 'mtu': '9100', 'tpid': '0x8100', 'alias': 'fortyGigE0/44', 'admin_status': 'up', 'autoneg': 'off', 'speed': '40000'}, "
                "'Ethernet48': {'lanes': '49,50,51,52', 'description': 'fortyGigE0/48', 'pfc_asym': 'off', 'mtu': '9100', 'tpid': '0x8100', 'alias': 'fortyGigE0/48', 'admin_status': 'up', 'autoneg': 'off', 'speed': '40000'}, "
                "'Ethernet52': {'lanes': '53,54,55,56', 'description': 'fortyGigE0/52', 'pfc_asym': 'off', 'mtu': '9100', 'tpid': '0x8100', 'alias': 'fortyGigE0/52', 'admin_status': 'up', 'autoneg': 'off', 'speed': '40000'}, "
                "'Ethernet56': {'lanes': '61,62,63,64', 'description': 'fortyGigE0/56', 'pfc_asym': 'off', 'mtu': '9100', 'tpid': '0x8100', 'alias': 'fortyGigE0/56', 'admin_status': 'up', 'autoneg': 'off', 'speed': '40000'}, "
                "'Ethernet60': {'lanes': '57,58,59,60', 'description': 'fortyGigE0/60', 'pfc_asym': 'off', 'mtu': '9100', 'tpid': '0x8100', 'alias': 'fortyGigE0/60', 'admin_status': 'up', 'autoneg': 'off', 'speed': '40000'}, "
                "'Ethernet64': {'lanes': '65,66,67,68', 'description': 'fortyGigE0/64', 'pfc_asym': 'off', 'mtu': '9100', 'tpid': '0x8100', 'alias': 'fortyGigE0/64', 'admin_status': 'up', 'autoneg': 'off', 'speed': '40000'}, "
                "'Ethernet68': {'lanes': '69,70,71,72', 'description': 'fortyGigE0/68', 'pfc_asym': 'off', 'mtu': '9100', 'tpid': '0x8100', 'alias': 'fortyGigE0/68', 'admin_status': 'up', 'autoneg': 'off', 'speed': '40000'}, "
                "'Ethernet72': {'lanes': '77,78,79,80', 'description': 'fortyGigE0/72', 'pfc_asym': 'off', 'mtu': '9100', 'tpid': '0x8100', 'alias': 'fortyGigE0/72', 'admin_status': 'up', 'autoneg': 'off', 'speed': '40000'}, "
                "'Ethernet76': {'lanes': '73,74,75,76', 'description': 'fortyGigE0/76', 'pfc_asym': 'off', 'mtu': '9100', 'tpid': '0x8100', 'alias': 'fortyGigE0/76', 'admin_status': 'up', 'autoneg': 'off', 'speed': '40000'}, "
                "'Ethernet80': {'lanes': '105,106,107,108', 'description': 'fortyGigE0/80', 'pfc_asym': 'off', 'mtu': '9100', 'tpid': '0x8100', 'alias': 'fortyGigE0/80', 'admin_status': 'up', 'autoneg': 'off', 'speed': '40000'}, "
                "'Ethernet84': {'lanes': '109,110,111,112', 'description': 'fortyGigE0/84', 'pfc_asym': 'off', 'mtu': '9100', 'tpid': '0x8100', 'alias': 'fortyGigE0/84', 'admin_status': 'up', 'autoneg': 'off', 'speed': '40000'}, "
                "'Ethernet88': {'lanes': '117,118,119,120', 'description': 'fortyGigE0/88', 'pfc_asym': 'off', 'mtu': '9100', 'tpid': '0x8100', 'alias': 'fortyGigE0/88', 'admin_status': 'up', 'autoneg': 'off', 'speed': '40000'}, "
                "'Ethernet92': {'lanes': '113,114,115,116', 'description': 'fortyGigE0/92', 'pfc_asym': 'off', 'mtu': '9100', 'tpid': '0x8100', 'alias': 'fortyGigE0/92', 'admin_status': 'up', 'autoneg': 'off', 'speed': '40000'}, "
                "'Ethernet96': {'lanes': '121,122,123,124', 'description': 'fortyGigE0/96', 'pfc_asym': 'off', 'mtu': '9100', 'tpid': '0x8100', 'alias': 'fortyGigE0/96', 'admin_status': 'up', 'autoneg': 'off', 'speed': '40000'}, "
                "'Ethernet100': {'lanes': '125,126,127,128', 'description': 'Servers100:eth0', 'pfc_asym': 'off', 'mtu': '9100', 'tpid': '0x8100', 'alias': 'fortyGigE0/100', 'admin_status': 'up', 'autoneg': 'off', 'speed': '40000'}, "
                "'Ethernet104': {'lanes': '85,86,87,88', 'description': 'fortyGigE0/104', 'mtu': '9100', 'tpid': '0x8100', 'alias': 'fortyGigE0/104', 'pfc_asym': 'off', 'autoneg': 'off', 'speed': '40000'}, "
                "'Ethernet108': {'lanes': '81,82,83,84', 'description': 'fortyGigE0/108', 'mtu': '9100', 'tpid': '0x8100', 'alias': 'fortyGigE0/108', 'pfc_asym': 'off', 'autoneg': 'off', 'speed': '40000'}, "
                "'Ethernet112': {'lanes': '89,90,91,92', 'description': 'ARISTA01T1:Ethernet1/1', 'pfc_asym': 'off', 'mtu': '9100', 'tpid': '0x8100', 'alias': 'fortyGigE0/112', 'admin_status': 'up', 'autoneg': 'on', 'speed': '40000'}, "
                "'Ethernet116': {'lanes': '93,94,95,96', 'description': 'ARISTA02T1:Ethernet1/1', 'pfc_asym': 'off', 'mtu': '9100', 'tpid': '0x8100', 'alias': 'fortyGigE0/116', 'admin_status': 'up', 'autoneg': 'on', 'speed': '40000'}, "
                "'Ethernet120': {'lanes': '97,98,99,100', 'description': 'ARISTA03T1:Ethernet1/1', 'pfc_asym': 'off', 'mtu': '9100', 'tpid': '0x8100', 'alias': 'fortyGigE0/120', 'admin_status': 'up', 'autoneg': 'on', 'speed': '40000'}, "
                "'Ethernet124': {'lanes': '101,102,103,104', 'fec': 'rs', 'pfc_asym': 'off', 'mtu': '9100', 'tpid': '0x8100', 'alias': 'fortyGigE0/124', 'admin_status': 'up', 'autoneg': 'on', 'speed': '100000', 'description': 'ARISTA04T1:Ethernet1/1', 'speed': '100000'}}"
            )
        )

    def test_minigraph_port_rs(self):
        argument = ['-m', self.sample_graph_t0, '-p', self.port_config, '-v', "PORT[\'Ethernet124\']"]
        output = self.run_script(argument)
        self.assertEqual(
            utils.to_dict(output.strip()),
            utils.to_dict("{'lanes': '101,102,103,104', 'fec': 'rs', 'pfc_asym': 'off', 'mtu': '9100', 'tpid': '0x8100', 'alias': 'fortyGigE0/124', 'admin_status': 'up', 'speed': '100000', 'description': 'ARISTA04T1:Ethernet1/1'}")
        )

    def test_minigraph_default_vxlan(self):
        argument = ['-m', self.sample_graph_deployment_id, '-p', self.port_config, '-v', "VXLAN_TUNNEL"]
        output = self.run_script(argument, False, False)
        self.assertEqual(
            utils.to_dict(output.strip()),
            utils.to_dict("{'tunnel_v4': {'src_ip': '10.1.0.32'}}")
        )

    def test_minigraph_default_vnet(self):
        argument = ['-m', self.sample_graph_deployment_id, '-p', self.port_config, '-v', "VNET"]
        output = self.run_script(argument, False, False)
        self.assertEqual(
            utils.to_dict(output.strip()),
            utils.to_dict("{'Vnet-default': {'vxlan_tunnel': 'tunnel_v4', 'scope': 'default', 'vni': 8000}}")
        )

    def test_minigraph_bgp(self):
        argument = ['-m', self.sample_graph_bgp_speaker, '-p', self.port_config, '-v', "BGP_NEIGHBOR[\'10.0.0.59\']"]
        output = self.run_script(argument)
        self.assertEqual(
            utils.to_dict(output.strip()),
            utils.to_dict("{'rrclient': 0, 'name': 'ARISTA02T1', 'local_addr': '10.0.0.58', 'nhopself': 0, 'holdtime': '180', 'asn': '64600', 'keepalive': '60'}")
        )

    def test_minigraph_peers_with_range(self):
        argument = ["-m", self.sample_graph_bgp_speaker, "-p", self.port_config, "-v", "BGP_PEER_RANGE.values()|list"]
        output = self.run_script(argument)
        self.assertEqual(
            utils.liststr_to_dict(output.strip()),
            utils.liststr_to_dict("[{'src_address': '10.1.0.32', 'name': 'BGPSLBPassive', 'ip_range': ['10.10.10.10/26', '100.100.100.100/26']}]")
        )

    def test_minigraph_deployment_id(self):
        argument = ['-m', self.sample_graph_bgp_speaker, '-p', self.port_config, '-v', "DEVICE_METADATA[\'localhost\'][\'deployment_id\']"]
        output = self.run_script(argument)
        self.assertEqual(output.strip(), "1")

    def test_minigraph_deployment_id_null(self):
        argument = ['-m', self.sample_graph_deployment_id, '-p', self.port_config, '-v', "DEVICE_METADATA[\'localhost\']"]
        output = self.run_script(argument)
        self.assertNotIn('deployment_id', output.strip())

    def test_minigraph_ethernet_interfaces(self, **kwargs):
        graph_file = kwargs.get('graph_file', self.sample_graph_simple)
        argument = ['-m', graph_file, '-p', self.port_config, '-v', "PORT[\'Ethernet8\']"]
        output = self.run_script(argument)
        self.assertEqual(
            utils.to_dict(output.strip()),
            utils.to_dict("{'lanes': '37,38,39,40', 'description': 'Interface description', 'pfc_asym': 'off', 'mtu': '9100', 'alias': 'fortyGigE0/8', 'admin_status': 'up', 'speed': '1000', 'tpid': '0x8100'}")
        )
        argument = ['-m', graph_file, '-p', self.port_config, '-v', "PORT[\'Ethernet12\']"]
        output = self.run_script(argument)
        self.assertEqual(
            utils.to_dict(output.strip()),
            utils.to_dict("{'lanes': '33,34,35,36', 'fec': 'rs', 'pfc_asym': 'off', 'mtu': '9100', 'tpid': '0x8100', 'alias': 'fortyGigE0/12', 'admin_status': 'up', 'speed': '100000', 'description': 'Interface description'}")
        )

    def test_minigraph_neighbor_interfaces(self):
        argument = ['-m', self.sample_graph_simple_case, '-p', self.port_config, '-v', "PORT"]
        output = self.run_script(argument)

        self.assertEqual(
            utils.to_dict(output.strip()),
            utils.to_dict(
                "{'Ethernet0': {'lanes': '29,30,31,32', 'description': 'switch-01t1:port1', 'pfc_asym': 'off', 'mtu': '9100', 'tpid': '0x8100', 'alias': 'fortyGigE0/0', 'admin_status': 'up', 'speed': '10000', 'autoneg': 'on'}, "
                "'Ethernet4': {'lanes': '25,26,27,28', 'description': 'server1:port1', 'pfc_asym': 'off', 'mtu': '9100', 'tpid': '0x8100', 'alias': 'fortyGigE0/4', 'admin_status': 'up', 'speed': '25000', 'autoneg': 'on', 'mux_cable': 'true'}, "
                "'Ethernet8': {'lanes': '37,38,39,40', 'description': 'Interface description', 'pfc_asym': 'off', 'mtu': '9100', 'tpid': '0x8100', 'alias': 'fortyGigE0/8', 'admin_status': 'up', 'speed': '40000', 'autoneg': 'on', 'mux_cable': 'true'}, "
                "'Ethernet12': {'lanes': '33,34,35,36', 'description': 'Interface description', 'pfc_asym': 'off', 'mtu': '9100', 'tpid': '0x8100', 'alias': 'fortyGigE0/12', 'admin_status': 'up', 'speed': '10000', 'autoneg': 'on'}, "
                "'Ethernet16': {'alias': 'fortyGigE0/16', 'pfc_asym': 'off', 'lanes': '41,42,43,44', 'description': 'fortyGigE0/16', 'mtu': '9100', 'tpid': '0x8100', 'fec': 'rs', 'speed': '100000'}, "
                "'Ethernet20': {'alias': 'fortyGigE0/20', 'pfc_asym': 'off', 'lanes': '45,46,47,48', 'description': 'fortyGigE0/20', 'mtu': '9100', 'tpid': '0x8100', 'fec': 'rs', 'speed': '100000'}, "
                "'Ethernet24': {'alias': 'fortyGigE0/24', 'pfc_asym': 'off', 'lanes': '5,6,7,8', 'description': 'fortyGigE0/24', 'mtu': '9100', 'tpid': '0x8100', 'fec': 'rs', 'speed': '100000'}, "
                "'Ethernet28': {'alias': 'fortyGigE0/28', 'pfc_asym': 'off', 'lanes': '1,2,3,4', 'description': 'fortyGigE0/28', 'mtu': '9100', 'tpid': '0x8100', 'fec': 'rs', 'speed': '100000'}, "
                "'Ethernet32': {'alias': 'fortyGigE0/32', 'pfc_asym': 'off', 'lanes': '9,10,11,12', 'description': 'fortyGigE0/32', 'mtu': '9100', 'tpid': '0x8100', 'fec': 'rs', 'speed': '100000'}, "
                "'Ethernet36': {'alias': 'fortyGigE0/36', 'pfc_asym': 'off', 'lanes': '13,14,15,16', 'description': 'fortyGigE0/36', 'mtu': '9100', 'tpid': '0x8100', 'fec': 'rs', 'speed': '100000'}, "
                "'Ethernet40': {'alias': 'fortyGigE0/40', 'pfc_asym': 'off', 'lanes': '21,22,23,24', 'description': 'fortyGigE0/40', 'mtu': '9100', 'tpid': '0x8100', 'fec': 'rs', 'speed': '100000'}, "
                "'Ethernet44': {'alias': 'fortyGigE0/44', 'pfc_asym': 'off', 'lanes': '17,18,19,20', 'description': 'fortyGigE0/44', 'mtu': '9100', 'tpid': '0x8100', 'fec': 'rs', 'speed': '100000'}, "
                "'Ethernet48': {'alias': 'fortyGigE0/48', 'pfc_asym': 'off', 'lanes': '49,50,51,52', 'description': 'fortyGigE0/48', 'mtu': '9100', 'tpid': '0x8100', 'fec': 'rs', 'speed': '100000'}, "
                "'Ethernet52': {'alias': 'fortyGigE0/52', 'pfc_asym': 'off', 'lanes': '53,54,55,56', 'description': 'fortyGigE0/52', 'mtu': '9100', 'tpid': '0x8100', 'fec': 'rs', 'speed': '100000'}, "
                "'Ethernet56': {'alias': 'fortyGigE0/56', 'pfc_asym': 'off', 'lanes': '61,62,63,64', 'description': 'fortyGigE0/56', 'mtu': '9100', 'tpid': '0x8100', 'fec': 'rs', 'speed': '100000'}, "
                "'Ethernet60': {'alias': 'fortyGigE0/60', 'pfc_asym': 'off', 'lanes': '57,58,59,60', 'description': 'fortyGigE0/60', 'mtu': '9100', 'tpid': '0x8100', 'fec': 'rs', 'speed': '100000'}, "
                "'Ethernet64': {'alias': 'fortyGigE0/64', 'pfc_asym': 'off', 'lanes': '65,66,67,68', 'description': 'fortyGigE0/64', 'mtu': '9100', 'tpid': '0x8100', 'fec': 'rs', 'speed': '100000'}, "
                "'Ethernet68': {'alias': 'fortyGigE0/68', 'pfc_asym': 'off', 'lanes': '69,70,71,72', 'description': 'fortyGigE0/68', 'mtu': '9100', 'tpid': '0x8100', 'fec': 'rs', 'speed': '100000'}, "
                "'Ethernet72': {'alias': 'fortyGigE0/72', 'pfc_asym': 'off', 'lanes': '77,78,79,80', 'description': 'fortyGigE0/72', 'mtu': '9100', 'tpid': '0x8100', 'fec': 'rs', 'speed': '100000'}, "
                "'Ethernet76': {'alias': 'fortyGigE0/76', 'pfc_asym': 'off', 'lanes': '73,74,75,76', 'description': 'fortyGigE0/76', 'mtu': '9100', 'tpid': '0x8100', 'fec': 'rs', 'speed': '100000'}, "
                "'Ethernet80': {'alias': 'fortyGigE0/80', 'pfc_asym': 'off', 'lanes': '105,106,107,108', 'description': 'fortyGigE0/80', 'mtu': '9100', 'tpid': '0x8100', 'fec': 'rs', 'speed': '100000'}, "
                "'Ethernet84': {'alias': 'fortyGigE0/84', 'pfc_asym': 'off', 'lanes': '109,110,111,112', 'description': 'fortyGigE0/84', 'mtu': '9100', 'tpid': '0x8100', 'fec': 'rs', 'speed': '100000'}, "
                "'Ethernet88': {'alias': 'fortyGigE0/88', 'pfc_asym': 'off', 'lanes': '117,118,119,120', 'description': 'fortyGigE0/88', 'mtu': '9100', 'tpid': '0x8100', 'fec': 'rs', 'speed': '100000'}, "
                "'Ethernet92': {'alias': 'fortyGigE0/92', 'pfc_asym': 'off', 'lanes': '113,114,115,116', 'description': 'fortyGigE0/92', 'mtu': '9100', 'tpid': '0x8100', 'fec': 'rs', 'speed': '100000'}, "
                "'Ethernet96': {'alias': 'fortyGigE0/96', 'pfc_asym': 'off', 'lanes': '121,122,123,124', 'description': 'fortyGigE0/96', 'mtu': '9100', 'tpid': '0x8100', 'fec': 'rs', 'speed': '100000'}, "
                "'Ethernet100': {'alias': 'fortyGigE0/100', 'pfc_asym': 'off', 'lanes': '125,126,127,128', 'description': 'fortyGigE0/100', 'mtu': '9100', 'tpid': '0x8100', 'fec': 'rs', 'speed': '100000'}, "
                "'Ethernet104': {'alias': 'fortyGigE0/104', 'pfc_asym': 'off', 'lanes': '85,86,87,88', 'description': 'fortyGigE0/104', 'mtu': '9100', 'tpid': '0x8100', 'fec': 'rs', 'speed': '100000'}, "
                "'Ethernet108': {'alias': 'fortyGigE0/108', 'pfc_asym': 'off', 'lanes': '81,82,83,84', 'description': 'fortyGigE0/108', 'mtu': '9100', 'tpid': '0x8100', 'fec': 'rs', 'speed': '100000'}, "
                "'Ethernet112': {'alias': 'fortyGigE0/112', 'pfc_asym': 'off', 'lanes': '89,90,91,92', 'description': 'fortyGigE0/112', 'mtu': '9100', 'tpid': '0x8100', 'fec': 'rs', 'speed': '100000'}, "
                "'Ethernet116': {'alias': 'fortyGigE0/116', 'pfc_asym': 'off', 'lanes': '93,94,95,96', 'description': 'fortyGigE0/116', 'mtu': '9100', 'tpid': '0x8100', 'fec': 'rs', 'speed': '100000'}, "
                "'Ethernet120': {'alias': 'fortyGigE0/120', 'pfc_asym': 'off', 'lanes': '97,98,99,100', 'description': 'fortyGigE0/120', 'mtu': '9100', 'tpid': '0x8100', 'fec': 'rs', 'speed': '100000'}, "
                "'Ethernet124': {'alias': 'fortyGigE0/124', 'pfc_asym': 'off', 'lanes': '101,102,103,104', 'description': 'fortyGigE0/124', 'mtu': '9100', 'tpid': '0x8100', 'fec': 'rs', 'speed': '100000'}}"
            )
        )

    def test_minigraph_neighbor_interfaces_config_db(self):
        # test to check if PORT table is retrieved from config_db
        argument = ['-m', self.sample_graph_simple_case, '-p', self.port_config, '-v', "PORT"]
        output = self.run_script(argument)

        self.assertEqual(
            utils.to_dict(output.strip()),
            utils.to_dict(
                "{'Ethernet0': {'lanes': '29,30,31,32', 'description': 'switch-01t1:port1', 'pfc_asym': 'off', 'mtu': '9100', 'tpid': '0x8100', 'alias': 'fortyGigE0/0', 'admin_status': 'up', 'speed': '10000', 'autoneg': 'on'}, "
                "'Ethernet4': {'lanes': '25,26,27,28', 'description': 'server1:port1', 'pfc_asym': 'off', 'mtu': '9100', 'tpid': '0x8100', 'alias': 'fortyGigE0/4', 'admin_status': 'up', 'speed': '25000', 'autoneg': 'on', 'mux_cable': 'true'}, "
                "'Ethernet8': {'lanes': '37,38,39,40', 'description': 'Interface description', 'pfc_asym': 'off', 'mtu': '9100', 'tpid': '0x8100', 'alias': 'fortyGigE0/8', 'admin_status': 'up', 'speed': '40000', 'autoneg': 'on', 'mux_cable': 'true'}, "
                "'Ethernet12': {'lanes': '33,34,35,36', 'description': 'Interface description', 'pfc_asym': 'off', 'mtu': '9100', 'tpid': '0x8100', 'alias': 'fortyGigE0/12', 'admin_status': 'up', 'speed': '10000', 'autoneg': 'on'}, "
                "'Ethernet16': {'alias': 'fortyGigE0/16', 'pfc_asym': 'off', 'lanes': '41,42,43,44', 'description': 'fortyGigE0/16', 'mtu': '9100', 'tpid': '0x8100', 'fec': 'rs', 'speed': '100000'}, "
                "'Ethernet20': {'alias': 'fortyGigE0/20', 'pfc_asym': 'off', 'lanes': '45,46,47,48', 'description': 'fortyGigE0/20', 'mtu': '9100', 'tpid': '0x8100', 'fec': 'rs', 'speed': '100000'}, "
                "'Ethernet24': {'alias': 'fortyGigE0/24', 'pfc_asym': 'off', 'lanes': '5,6,7,8', 'description': 'fortyGigE0/24', 'mtu': '9100', 'tpid': '0x8100', 'fec': 'rs', 'speed': '100000'}, "
                "'Ethernet28': {'alias': 'fortyGigE0/28', 'pfc_asym': 'off', 'lanes': '1,2,3,4', 'description': 'fortyGigE0/28', 'mtu': '9100', 'tpid': '0x8100', 'fec': 'rs', 'speed': '100000'}, "
                "'Ethernet32': {'alias': 'fortyGigE0/32', 'pfc_asym': 'off', 'lanes': '9,10,11,12', 'description': 'fortyGigE0/32', 'mtu': '9100', 'tpid': '0x8100', 'fec': 'rs', 'speed': '100000'}, "
                "'Ethernet36': {'alias': 'fortyGigE0/36', 'pfc_asym': 'off', 'lanes': '13,14,15,16', 'description': 'fortyGigE0/36', 'mtu': '9100', 'tpid': '0x8100', 'fec': 'rs', 'speed': '100000'}, "
                "'Ethernet40': {'alias': 'fortyGigE0/40', 'pfc_asym': 'off', 'lanes': '21,22,23,24', 'description': 'fortyGigE0/40', 'mtu': '9100', 'tpid': '0x8100', 'fec': 'rs', 'speed': '100000'}, "
                "'Ethernet44': {'alias': 'fortyGigE0/44', 'pfc_asym': 'off', 'lanes': '17,18,19,20', 'description': 'fortyGigE0/44', 'mtu': '9100', 'tpid': '0x8100', 'fec': 'rs', 'speed': '100000'}, "
                "'Ethernet48': {'alias': 'fortyGigE0/48', 'pfc_asym': 'off', 'lanes': '49,50,51,52', 'description': 'fortyGigE0/48', 'mtu': '9100', 'tpid': '0x8100', 'fec': 'rs', 'speed': '100000'}, "
                "'Ethernet52': {'alias': 'fortyGigE0/52', 'pfc_asym': 'off', 'lanes': '53,54,55,56', 'description': 'fortyGigE0/52', 'mtu': '9100', 'tpid': '0x8100', 'fec': 'rs', 'speed': '100000'}, "
                "'Ethernet56': {'alias': 'fortyGigE0/56', 'pfc_asym': 'off', 'lanes': '61,62,63,64', 'description': 'fortyGigE0/56', 'mtu': '9100', 'tpid': '0x8100', 'fec': 'rs', 'speed': '100000'}, "
                "'Ethernet60': {'alias': 'fortyGigE0/60', 'pfc_asym': 'off', 'lanes': '57,58,59,60', 'description': 'fortyGigE0/60', 'mtu': '9100', 'tpid': '0x8100', 'fec': 'rs', 'speed': '100000'}, "
                "'Ethernet64': {'alias': 'fortyGigE0/64', 'pfc_asym': 'off', 'lanes': '65,66,67,68', 'description': 'fortyGigE0/64', 'mtu': '9100', 'tpid': '0x8100', 'fec': 'rs', 'speed': '100000'}, "
                "'Ethernet68': {'alias': 'fortyGigE0/68', 'pfc_asym': 'off', 'lanes': '69,70,71,72', 'description': 'fortyGigE0/68', 'mtu': '9100', 'tpid': '0x8100', 'fec': 'rs', 'speed': '100000'}, "
                "'Ethernet72': {'alias': 'fortyGigE0/72', 'pfc_asym': 'off', 'lanes': '77,78,79,80', 'description': 'fortyGigE0/72', 'mtu': '9100', 'tpid': '0x8100', 'fec': 'rs', 'speed': '100000'}, "
                "'Ethernet76': {'alias': 'fortyGigE0/76', 'pfc_asym': 'off', 'lanes': '73,74,75,76', 'description': 'fortyGigE0/76', 'mtu': '9100', 'tpid': '0x8100', 'fec': 'rs', 'speed': '100000'}, "
                "'Ethernet80': {'alias': 'fortyGigE0/80', 'pfc_asym': 'off', 'lanes': '105,106,107,108', 'description': 'fortyGigE0/80', 'mtu': '9100', 'tpid': '0x8100', 'fec': 'rs', 'speed': '100000'}, "
                "'Ethernet84': {'alias': 'fortyGigE0/84', 'pfc_asym': 'off', 'lanes': '109,110,111,112', 'description': 'fortyGigE0/84', 'mtu': '9100', 'tpid': '0x8100', 'fec': 'rs', 'speed': '100000'}, "
                "'Ethernet88': {'alias': 'fortyGigE0/88', 'pfc_asym': 'off', 'lanes': '117,118,119,120', 'description': 'fortyGigE0/88', 'mtu': '9100', 'tpid': '0x8100', 'fec': 'rs', 'speed': '100000'}, "
                "'Ethernet92': {'alias': 'fortyGigE0/92', 'pfc_asym': 'off', 'lanes': '113,114,115,116', 'description': 'fortyGigE0/92', 'mtu': '9100', 'tpid': '0x8100', 'fec': 'rs', 'speed': '100000'}, "
                "'Ethernet96': {'alias': 'fortyGigE0/96', 'pfc_asym': 'off', 'lanes': '121,122,123,124', 'description': 'fortyGigE0/96', 'mtu': '9100', 'tpid': '0x8100', 'fec': 'rs', 'speed': '100000'}, "
                "'Ethernet100': {'alias': 'fortyGigE0/100', 'pfc_asym': 'off', 'lanes': '125,126,127,128', 'description': 'fortyGigE0/100', 'mtu': '9100', 'tpid': '0x8100', 'fec': 'rs', 'speed': '100000'}, "
                "'Ethernet104': {'alias': 'fortyGigE0/104', 'pfc_asym': 'off', 'lanes': '85,86,87,88', 'description': 'fortyGigE0/104', 'mtu': '9100', 'tpid': '0x8100', 'fec': 'rs', 'speed': '100000'}, "
                "'Ethernet108': {'alias': 'fortyGigE0/108', 'pfc_asym': 'off', 'lanes': '81,82,83,84', 'description': 'fortyGigE0/108', 'mtu': '9100', 'tpid': '0x8100', 'fec': 'rs', 'speed': '100000'}, "
                "'Ethernet112': {'alias': 'fortyGigE0/112', 'pfc_asym': 'off', 'lanes': '89,90,91,92', 'description': 'fortyGigE0/112', 'mtu': '9100', 'tpid': '0x8100', 'fec': 'rs', 'speed': '100000'}, "
                "'Ethernet116': {'alias': 'fortyGigE0/116', 'pfc_asym': 'off', 'lanes': '93,94,95,96', 'description': 'fortyGigE0/116', 'mtu': '9100', 'tpid': '0x8100', 'fec': 'rs', 'speed': '100000'}, "
                "'Ethernet120': {'alias': 'fortyGigE0/120', 'pfc_asym': 'off', 'lanes': '97,98,99,100', 'description': 'fortyGigE0/120', 'mtu': '9100', 'tpid': '0x8100', 'fec': 'rs', 'speed': '100000'}, "
                "'Ethernet124': {'alias': 'fortyGigE0/124', 'pfc_asym': 'off', 'lanes': '101,102,103,104', 'description': 'fortyGigE0/124', 'mtu': '9100', 'tpid': '0x8100', 'fec': 'rs', 'speed': '100000'}}"
            )
        )

    def test_minigraph_extra_ethernet_interfaces(self, **kwargs):
        graph_file = kwargs.get('graph_file', self.sample_graph_simple)
        argument = ['-m', graph_file, '-p', self.port_config, '-v', "PORT"]
        output = self.run_script(argument)

        self.assertEqual(
            utils.to_dict(output.strip()),
            utils.to_dict(
                "{'Ethernet0': {'lanes': '29,30,31,32', 'description': 'fortyGigE0/0', 'pfc_asym': 'off', 'mtu': '9100', 'tpid': '0x8100', 'alias': 'fortyGigE0/0', 'admin_status': 'up', 'speed': '10000'}, "
                "'Ethernet4': {'lanes': '25,26,27,28', 'description': 'fortyGigE0/4', 'pfc_asym': 'off', 'mtu': '9100', 'tpid': '0x8100', 'alias': 'fortyGigE0/4', 'admin_status': 'up', 'speed': '25000'}, "
                "'Ethernet8': {'lanes': '37,38,39,40', 'description': 'Interface description', 'pfc_asym': 'off', 'mtu': '9100', 'tpid': '0x8100', 'alias': 'fortyGigE0/8', 'admin_status': 'up', 'speed': '1000'}, "
                "'Ethernet12': {'lanes': '33,34,35,36', 'fec': 'rs', 'pfc_asym': 'off', 'mtu': '9100', 'tpid': '0x8100', 'alias': 'fortyGigE0/12', 'admin_status': 'up', 'speed': '100000', 'description': 'Interface description'}, "
                "'Ethernet16': {'lanes': '41,42,43,44', 'pfc_asym': 'off', 'description': 'fortyGigE0/16', 'mtu': '9100', 'tpid': '0x8100', 'alias': 'fortyGigE0/16', 'speed': '1000'}, "
                "'Ethernet20': {'alias': 'fortyGigE0/20', 'pfc_asym': 'off', 'lanes': '45,46,47,48', 'description': 'fortyGigE0/20', 'mtu': '9100', 'tpid': '0x8100', 'fec': 'rs', 'speed': '100000'}, "
                "'Ethernet24': {'alias': 'fortyGigE0/24', 'pfc_asym': 'off', 'lanes': '5,6,7,8', 'description': 'fortyGigE0/24', 'mtu': '9100', 'tpid': '0x8100', 'fec': 'rs', 'speed': '100000'}, "
                "'Ethernet28': {'alias': 'fortyGigE0/28', 'pfc_asym': 'off', 'lanes': '1,2,3,4', 'description': 'fortyGigE0/28', 'mtu': '9100', 'tpid': '0x8100', 'fec': 'rs', 'speed': '100000'}, "
                "'Ethernet32': {'alias': 'fortyGigE0/32', 'pfc_asym': 'off', 'lanes': '9,10,11,12', 'description': 'fortyGigE0/32', 'mtu': '9100', 'tpid': '0x8100', 'fec': 'rs', 'speed': '100000'}, "
                "'Ethernet36': {'alias': 'fortyGigE0/36', 'pfc_asym': 'off', 'lanes': '13,14,15,16', 'description': 'fortyGigE0/36', 'mtu': '9100', 'tpid': '0x8100', 'fec': 'rs', 'speed': '100000'}, "
                "'Ethernet40': {'alias': 'fortyGigE0/40', 'pfc_asym': 'off', 'lanes': '21,22,23,24', 'description': 'fortyGigE0/40', 'mtu': '9100', 'tpid': '0x8100', 'fec': 'rs', 'speed': '100000'}, "
                "'Ethernet44': {'alias': 'fortyGigE0/44', 'pfc_asym': 'off', 'lanes': '17,18,19,20', 'description': 'fortyGigE0/44', 'mtu': '9100', 'tpid': '0x8100', 'fec': 'rs', 'speed': '100000'}, "
                "'Ethernet48': {'alias': 'fortyGigE0/48', 'pfc_asym': 'off', 'lanes': '49,50,51,52', 'description': 'fortyGigE0/48', 'mtu': '9100', 'tpid': '0x8100', 'fec': 'rs', 'speed': '100000'}, "
                "'Ethernet52': {'alias': 'fortyGigE0/52', 'pfc_asym': 'off', 'lanes': '53,54,55,56', 'description': 'fortyGigE0/52', 'mtu': '9100', 'tpid': '0x8100', 'fec': 'rs', 'speed': '100000'}, "
                "'Ethernet56': {'alias': 'fortyGigE0/56', 'pfc_asym': 'off', 'lanes': '61,62,63,64', 'description': 'fortyGigE0/56', 'mtu': '9100', 'tpid': '0x8100', 'fec': 'rs', 'speed': '100000'}, "
                "'Ethernet60': {'alias': 'fortyGigE0/60', 'pfc_asym': 'off', 'lanes': '57,58,59,60', 'description': 'fortyGigE0/60', 'mtu': '9100', 'tpid': '0x8100', 'fec': 'rs', 'speed': '100000'}, "
                "'Ethernet64': {'alias': 'fortyGigE0/64', 'pfc_asym': 'off', 'lanes': '65,66,67,68', 'description': 'fortyGigE0/64', 'mtu': '9100', 'tpid': '0x8100', 'fec': 'rs', 'speed': '100000'}, "
                "'Ethernet68': {'alias': 'fortyGigE0/68', 'pfc_asym': 'off', 'lanes': '69,70,71,72', 'description': 'fortyGigE0/68', 'mtu': '9100', 'tpid': '0x8100', 'fec': 'rs', 'speed': '100000'}, "
                "'Ethernet72': {'alias': 'fortyGigE0/72', 'pfc_asym': 'off', 'lanes': '77,78,79,80', 'description': 'fortyGigE0/72', 'mtu': '9100', 'tpid': '0x8100', 'fec': 'rs', 'speed': '100000'}, "
                "'Ethernet76': {'alias': 'fortyGigE0/76', 'pfc_asym': 'off', 'lanes': '73,74,75,76', 'description': 'fortyGigE0/76', 'mtu': '9100', 'tpid': '0x8100', 'fec': 'rs', 'speed': '100000'}, "
                "'Ethernet80': {'alias': 'fortyGigE0/80', 'pfc_asym': 'off', 'lanes': '105,106,107,108', 'description': 'fortyGigE0/80', 'mtu': '9100', 'tpid': '0x8100', 'fec': 'rs', 'speed': '100000'}, "
                "'Ethernet84': {'alias': 'fortyGigE0/84', 'pfc_asym': 'off', 'lanes': '109,110,111,112', 'description': 'fortyGigE0/84', 'mtu': '9100', 'tpid': '0x8100', 'fec': 'rs', 'speed': '100000'}, "
                "'Ethernet88': {'alias': 'fortyGigE0/88', 'pfc_asym': 'off', 'lanes': '117,118,119,120', 'description': 'fortyGigE0/88', 'mtu': '9100', 'tpid': '0x8100', 'fec': 'rs', 'speed': '100000'}, "
                "'Ethernet92': {'alias': 'fortyGigE0/92', 'pfc_asym': 'off', 'lanes': '113,114,115,116', 'description': 'fortyGigE0/92', 'mtu': '9100', 'tpid': '0x8100', 'fec': 'rs', 'speed': '100000'}, "
                "'Ethernet96': {'alias': 'fortyGigE0/96', 'pfc_asym': 'off', 'lanes': '121,122,123,124', 'description': 'fortyGigE0/96', 'mtu': '9100', 'tpid': '0x8100', 'fec': 'rs', 'speed': '100000'}, "
                "'Ethernet100': {'alias': 'fortyGigE0/100', 'pfc_asym': 'off', 'lanes': '125,126,127,128', 'description': 'fortyGigE0/100', 'mtu': '9100', 'tpid': '0x8100', 'fec': 'rs', 'speed': '100000'}, "
                "'Ethernet104': {'alias': 'fortyGigE0/104', 'pfc_asym': 'off', 'lanes': '85,86,87,88', 'description': 'fortyGigE0/104', 'mtu': '9100', 'tpid': '0x8100', 'fec': 'rs', 'speed': '100000'}, "
                "'Ethernet108': {'alias': 'fortyGigE0/108', 'pfc_asym': 'off', 'lanes': '81,82,83,84', 'description': 'fortyGigE0/108', 'mtu': '9100', 'tpid': '0x8100', 'fec': 'rs', 'speed': '100000'}, "
                "'Ethernet112': {'alias': 'fortyGigE0/112', 'pfc_asym': 'off', 'lanes': '89,90,91,92', 'description': 'fortyGigE0/112', 'mtu': '9100', 'tpid': '0x8100', 'fec': 'rs', 'speed': '100000'}, "
                "'Ethernet116': {'alias': 'fortyGigE0/116', 'pfc_asym': 'off', 'lanes': '93,94,95,96', 'description': 'fortyGigE0/116', 'mtu': '9100', 'tpid': '0x8100', 'fec': 'rs', 'speed': '100000'}, "
                "'Ethernet120': {'alias': 'fortyGigE0/120', 'pfc_asym': 'off', 'lanes': '97,98,99,100', 'description': 'fortyGigE0/120', 'mtu': '9100', 'tpid': '0x8100', 'fec': 'rs', 'speed': '100000'}, "
                "'Ethernet124': {'alias': 'fortyGigE0/124', 'pfc_asym': 'off', 'lanes': '101,102,103,104', 'description': 'fortyGigE0/124', 'mtu': '9100', 'tpid': '0x8100', 'fec': 'rs', 'speed': '100000'}}"
            )
        )

#     everflow portion is not used
#     def test_metadata_everflow(self):
#         argument = '-m "' + self.sample_graph_metadata + '" -p "' + self.port_config + '" -v "MIRROR_SESSION"'
#         output = self.run_script(argument)
#         self.assertEqual(output.strip(), "{'everflow0': {'src_ip': '10.1.0.32', 'dst_ip': '10.0.100.1'}}")

    def test_metadata_tacacs(self):
        argument = ['-m', self.sample_graph_metadata, '-p', self.port_config, '-v', "TACPLUS_SERVER"]
        output = self.run_script(argument)
        self.assertEqual(
            utils.to_dict(output.strip()),
            utils.to_dict("{'10.0.10.7': {'priority': '1', 'tcp_port': '49'}, '10.0.10.8': {'priority': '1', 'tcp_port': '49'}}")
        )

    def test_metadata_ntp(self):
        argument = ['-m', self.sample_graph_metadata, '-p', self.port_config, '-v', "NTP_SERVER"]
        output = self.run_script(argument)
        self.assertEqual(utils.to_dict(output.strip()), utils.to_dict("{'10.0.10.1': {'iburst': 'on'}, '10.0.10.2': {'iburst': 'on'}}"))

    def test_dns_nameserver(self):
        argument = ['-m', self.sample_graph_metadata, '-p', self.port_config, '-v', "DNS_NAMESERVER"]
        output = self.run_script(argument)
        self.assertEqual(utils.to_dict(output.strip()), utils.to_dict("{'6.6.6.6': {}}"))

    def test_minigraph_vnet(self, **kwargs):
        graph_file = kwargs.get('graph_file', self.sample_graph_simple)
        argument = ['-m', graph_file, '-p', self.port_config, '-v', "VNET"]
        output = self.run_script(argument)
        self.assertEqual(output.strip(), "")

    def test_minigraph_vxlan(self, **kwargs):
        graph_file = kwargs.get('graph_file', self.sample_graph_simple)
        argument = ['-m', graph_file, '-p', self.port_config, '-v', "VXLAN_TUNNEL"]
        output = self.run_script(argument)
        self.assertEqual(output.strip(), "")

    def test_minigraph_bgp_mon(self):
        argument = ['-m', self.sample_graph_simple, '-p', self.port_config, '-v', "BGP_MONITORS"]
        output = self.run_script(argument)
        self.assertEqual(
            utils.to_dict(output.strip()),
            utils.to_dict("{'10.20.30.40': {'rrclient': 0, 'name': 'BGPMonitor', 'local_addr': '10.1.0.32', 'nhopself': 0, 'holdtime': '10', 'asn': '0', 'keepalive': '3'}}")
        )

    def test_minigraph_bgp_voq_chassis_peer(self):
        argument = ['-m', self.sample_graph_simple, '-p', self.port_config, '-v', "BGP_VOQ_CHASSIS_NEIGHBOR[\'10.2.0.21\']"]
        output = self.run_script(argument)
        self.assertEqual(
            utils.to_dict(output.strip()),
            utils.to_dict("{'rrclient': 0, 'name': 'CHASSIS_PEER', 'local_addr': '10.2.0.20', 'nhopself': 0, 'holdtime': '180', 'asn': '65100', 'keepalive': '60', 'admin_status': 'up'}")
        )

        # make sure VoQChassisInternal value of false is honored
        argument = ['-m', self.sample_graph_simple, '-p', self.port_config, '-v', "BGP_VOQ_CHASSIS_NEIGHBOR[\'10.0.0.57\']"]
        output = self.run_script(argument)
        self.assertEqual(output.strip(), "")

    def test_minigraph_bgp_sentinel(self):
        argument = ['-m', self.sample_graph_simple, '-p', self.port_config, '-v', "BGP_SENTINELS[\'BGPSentinel\']"]
        output = self.run_script(argument)
        self.assertEqual(
            utils.to_dict(output.strip()),
            utils.to_dict("{'name': 'BGPSentinel', 'ip_range': ['10.154.232.0/21','10.42.168.0/21'], 'src_address': '10.2.0.20'}")
        )

        argument = ['-m', self.sample_graph_simple, '-p', self.port_config, '-v', "BGP_SENTINELS[\'BGPSentinelV6\']"]
        output = self.run_script(argument)
        self.assertEqual(
            utils.to_dict(output.strip()),
            utils.to_dict("{'name': 'BGPSentinelV6', 'ip_range': ['2603:10a0:321:82f9::/64','2603:10a1:30a:8000::/59'], 'src_address': 'fc00:1::32'}")
        )

    def test_minigraph_sub_port_intf_resource_type_non_backend_tor(self, check_stderr=True):
        self.verify_sub_intf_non_backend_tor(graph_file=self.sample_resource_graph, check_stderr=check_stderr)

    def test_minigraph_sub_port_intf_hwsku(self, check_stderr=True):
        self.verify_sub_intf(graph_file=self.sample_backend_graph, check_stderr=check_stderr)

    def test_minigraph_sub_port_intf_sub(self, check_stderr=True):
        self.verify_sub_intf(graph_file=self.sample_subintf_graph, check_stderr=check_stderr)

    def test_minigraph_no_vlan_member(self, check_stderr=True):
        self.verify_no_vlan_member()

    def test_minigraph_backend_acl_leaf(self, check_stderr=True):
        try:
            print('\n    Change device type to %s' % (BACKEND_LEAF_ROUTER))
            if check_stderr:
                output = subprocess.check_output(["sed", "-i", 's/%s/%s/g' % (TOR_ROUTER, BACKEND_LEAF_ROUTER), self.sample_backend_graph], stderr=subprocess.STDOUT)
            else:
                output = subprocess.check_output(["sed", "-i", 's/%s/%s/g' % (TOR_ROUTER, BACKEND_LEAF_ROUTER), self.sample_backend_graph])

            self.test_jinja_expression(self.sample_backend_graph, self.port_config, BACKEND_LEAF_ROUTER)

            # ACL_TABLE should contain EVERFLOW related entries
            argument = ['-m', self.sample_backend_graph, '-p', self.port_config, '-v', "ACL_TABLE"]
            output = self.run_script(argument)
            sample_output = utils.to_dict(output.strip()).keys()
            assert 'DATAACL' not in sample_output, sample_output
            assert 'EVERFLOW' in sample_output, sample_output

        finally:
            print('\n    Change device type back to %s' % (TOR_ROUTER))
            if check_stderr:
                output = subprocess.check_output(["sed", "-i", 's/%s/%s/g' % (BACKEND_LEAF_ROUTER, TOR_ROUTER), self.sample_backend_graph], stderr=subprocess.STDOUT)
            else:
                output = subprocess.check_output(["sed", "-i", 's/%s/%s/g' % (BACKEND_LEAF_ROUTER, TOR_ROUTER), self.sample_backend_graph])

            self.test_jinja_expression(self.sample_backend_graph, self.port_config, TOR_ROUTER)

    def test_minigraph_sub_port_no_vlan_member(self, check_stderr=True):
        try:
            print('\n    Change device type to %s' % (BACKEND_LEAF_ROUTER))
            if check_stderr:
                output = subprocess.check_output(["sed", "-i", 's/%s/%s/g' % (LEAF_ROUTER, BACKEND_LEAF_ROUTER), self.sample_graph], stderr=subprocess.STDOUT)
            else:
                output = subprocess.check_output(["sed", "-i", 's/%s/%s/g' % (LEAF_ROUTER, BACKEND_LEAF_ROUTER), self.sample_graph])

            self.test_jinja_expression(self.sample_graph, self.port_config, BACKEND_LEAF_ROUTER)
            self.verify_no_vlan_member()
        finally:
            print('\n    Change device type back to %s' % (LEAF_ROUTER))
            if check_stderr:
                output = subprocess.check_output(["sed", "-i", 's/%s/%s/g' % (BACKEND_LEAF_ROUTER, LEAF_ROUTER), self.sample_graph], stderr=subprocess.STDOUT)
            else:
                output = subprocess.check_output(["sed", "-i", 's/%s/%s/g' % (BACKEND_LEAF_ROUTER, LEAF_ROUTER), self.sample_graph])

            self.test_jinja_expression(self.sample_graph, self.port_config, LEAF_ROUTER)

    def verify_no_vlan_member(self):
        argument = ['-m', self.sample_graph, '-p', self.port_config, '-v', "VLAN_MEMBER"]
        output = self.run_script(argument)
        self.assertEqual(output.strip(), "{}")

    def verify_sub_intf_non_backend_tor(self, **kwargs):
        graph_file = kwargs.get('graph_file', self.sample_graph_simple)

        # All the other tables stay unchanged
        self.test_var_json_data(graph_file=graph_file)
        self.test_minigraph_vlans(graph_file=graph_file)
        self.test_minigraph_vlan_members(graph_file=graph_file)

    def verify_sub_intf(self, **kwargs):
        graph_file = kwargs.get('graph_file', self.sample_graph_simple)
        check_stderr = kwargs.get('check_stderr', True)
        try:
            print('\n    Change device type to %s' % (BACKEND_TOR_ROUTER))
            if check_stderr:
                output = subprocess.check_output(["sed", "-i", 's/%s/%s/g' % (TOR_ROUTER, BACKEND_TOR_ROUTER), graph_file], stderr=subprocess.STDOUT)
            else:
                output = subprocess.check_output(["sed", "-i", 's/%s/%s/g' % (TOR_ROUTER, BACKEND_TOR_ROUTER), graph_file])

            self.test_jinja_expression(graph_file, self.port_config, BACKEND_TOR_ROUTER)


            # INTERFACE table does not exist
            argument = ['-m', graph_file, '-p', self.port_config, '-v',  "INTERFACE"]
            output = self.run_script(argument)
            self.assertEqual(output.strip(), "")

            # PORTCHANNEL_INTERFACE table does not exist
            argument = ['-m', graph_file, '-p', self.port_config, '-v', "PORTCHANNEL_INTERFACE"]
            output = self.run_script(argument)
            self.assertEqual(output.strip(), "")

            # SLB and BGP Monitor table does not exist
            argument = ['-m', graph_file, '-p', self.port_config, '-v', "BGP_PEER_RANGE"]
            output = self.run_script(argument)
            self.assertEqual(output.strip(), "{}")
            argument = ['-m', graph_file, '-p', self.port_config, '-v', "BGP_MONITORS"]
            output = self.run_script(argument)
            self.assertEqual(output.strip(), "{}")

            # ACL_TABLE should not contain EVERFLOW related entries
            argument = ['-m', graph_file, '-p', self.port_config, '-v', "ACL_TABLE"]
            output = self.run_script(argument)
            sample_output = utils.to_dict(output.strip()).keys()
            assert 'DATAACL' in sample_output, sample_output
            assert 'EVERFLOW' not in sample_output, sample_output

            # All the other tables stay unchanged
            self.test_minigraph_vlans(graph_file=graph_file)
            self.test_minigraph_vlan_interfaces(graph_file=graph_file)
            self.test_minigraph_portchannels(graph_file=graph_file)
            self.test_minigraph_ethernet_interfaces(graph_file=graph_file)
            self.test_minigraph_extra_ethernet_interfaces(graph_file=graph_file)
            self.test_minigraph_vnet(graph_file=graph_file)
            self.test_minigraph_vxlan(graph_file=graph_file)

            # VLAN_SUB_INTERFACE
            argument = ['-m', graph_file, '-p', self.port_config, '-v', 'VLAN_SUB_INTERFACE']
            output = self.run_script(argument)
            print(output.strip())
            # not a usecase to parse SubInterfaces under PortChannel
            if 'subintf' in graph_file:
                self.assertEqual(
                    utils.to_dict(output.strip()),
                    utils.to_dict(
                        "{'Ethernet0.10': {'admin_status': 'up'}, "
                        "('Ethernet0.10', '10.0.0.58/31'): {}, "
                        "('Ethernet0.10', 'FC00::75/126'): {}}"
                    )
                )
            else:
                self.assertEqual(
                    utils.to_dict(output.strip()),
                    utils.to_dict(
                        "{('PortChannel1.10', '10.0.0.56/31'): {}, "
                        "'Ethernet0.10': {'admin_status': 'up'}, "
                        "('Ethernet0.10', '10.0.0.58/31'): {}, "
                        "('PortChannel1.10', 'FC00::71/126'): {}, "
                        "'PortChannel1.10': {'admin_status': 'up'}, "
                        "('Ethernet0.10', 'FC00::75/126'): {}}"
                    )
                )

            # VLAN_MEMBER table should have all tagged members
            self.test_var_json_data(graph_file=graph_file, tag_mode='tagged')
            self.test_minigraph_vlan_members(graph_file=graph_file, tag_mode='tagged')

        finally:
            print('\n    Change device type back to %s' % (TOR_ROUTER))
            if check_stderr:
                output = subprocess.check_output(["sed", "-i", 's/%s/%s/g' % (BACKEND_TOR_ROUTER, TOR_ROUTER), graph_file], stderr=subprocess.STDOUT)
            else:
                output = subprocess.check_output(["sed", "-i", 's/%s/%s/g' % (BACKEND_TOR_ROUTER, TOR_ROUTER), graph_file])

            self.test_jinja_expression(graph_file, self.port_config, TOR_ROUTER)

    def test_show_run_acl(self):
        argument = ['-a', '{"key1":"value"}', '--var-json', 'ACL_RULE']
        output = self.run_script(argument)
        self.assertEqual(output, '')

    def test_show_run_interfaces(self):
        argument = ['-a', '{"key1":"value"}', '--var-json', 'INTERFACE']
        output = self.run_script(argument)
        self.assertEqual(output, '')

    def test_minigraph_voq_metadata(self):
        argument = ["-j", self.macsec_profile, "-m", self.sample_graph_voq, "-p", self.voq_port_config, "--var-json", "DEVICE_METADATA"]
        output = json.loads(self.run_script(argument))
        self.assertEqual(output['localhost']['asic_name'], 'Asic0')
        self.assertEqual(output['localhost']['switch_id'], '0')
        self.assertEqual(output['localhost']['switch_type'], 'voq')
        self.assertEqual(output['localhost']['max_cores'], '16')

    def test_minigraph_voq_system_ports(self):
        argument = ["-j", self.macsec_profile, "-m", self.sample_graph_voq, "-p", self.voq_port_config, "--var-json", "SYSTEM_PORT"]
        self.assertDictEqual(
            json.loads(self.run_script(argument)),
            {
                "linecard-1|Asic0|Cpu0": { "core_port_index": "0", "num_voq": "8", "switch_id": "0", "speed": "1000", "core_index": "0", "system_port_id": "1" },
                "linecard-1|Asic0|Ethernet0": { "core_port_index": "1", "num_voq": "8", "switch_id": "0", "speed": "40000", "core_index": "0", "system_port_id": "2" },
                "linecard-1|Asic0|Ethernet4": { "core_port_index": "2", "num_voq": "8", "switch_id": "0", "speed": "40000", "core_index": "0", "system_port_id": "3" },
                "linecard-1|Asic0|Ethernet8": { "core_port_index": "3", "num_voq": "8", "switch_id": "0", "speed": "40000", "core_index": "1", "system_port_id": "4" },
                "linecard-1|Asic0|Ethernet12": { "core_port_index": "4", "num_voq": "8", "switch_id": "0", "speed": "40000", "core_index": "1", "system_port_id": "5" },
                "linecard-2|Asic0|Cpu0": { "core_port_index": "0", "num_voq": "8", "switch_id": "2", "speed": "1000", "core_index": "0", "system_port_id": "256" },
                "linecard-2|Asic0|Ethernet0": { "core_port_index": "1", "num_voq": "8", "switch_id": "2", "speed": "40000", "core_index": "0", "system_port_id": "257" },
                "linecard-2|Asic0|Ethernet4": { "core_port_index": "2", "num_voq": "8", "switch_id": "2", "speed": "40000", "core_index": "1", "system_port_id": "258" },
                "linecard-2|Asic1|Cpu0": { "core_port_index": "0", "num_voq": "8", "switch_id": "4", "speed": "1000", "core_index": "0", "system_port_id": "259" },
                "linecard-2|Asic1|Ethernet8": { "core_port_index": "1", "num_voq": "8", "switch_id": "4", "speed": "40000", "core_index": "0", "system_port_id": "260" },
                "linecard-2|Asic1|Ethernet12": { "core_port_index": "2", "num_voq": "8", "switch_id": "4", "speed": "40000", "core_index": "1", "system_port_id": "261" }
            }
        )

    def test_minigraph_voq_port_macsec_enabled(self):
        argument = ['-j', self.macsec_profile, '-m', self.sample_graph_voq, '-p', self.voq_port_config, '-v', "PORT[\'Ethernet0\']"]
        output = self.run_script(argument)
        self.assertEqual(
            utils.to_dict(output.strip()),
            utils.to_dict("{'lanes': '6,7', 'fec': 'rs', 'alias': 'Ethernet1/1', 'index': '1', 'role': 'Ext', 'speed': '100000', 'macsec': 'macsec-profile', 'description': 'Ethernet1/1', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off'}")
        )

    def test_minigraph_voq_inband_interface_port(self):
        argument = ["-j", self.macsec_profile, "-m", self.sample_graph_voq, "-p", self.voq_port_config, "--var-json", "VOQ_INBAND_INTERFACE"]
        output = self.run_script(argument)
        output_dict = utils.to_dict(output.strip())
        self.assertDictEqual(
            output_dict['Ethernet-IB0'],
            {'inband_type': 'port'}
        )
        self.assertDictEqual(
            output_dict['Ethernet-IB0|2.2.2.2/32'],
            {}
        )

    def test_minigraph_voq_inband_port(self):
        argument = ["-j", self.macsec_profile, "-m", self.sample_graph_voq, "-p", self.voq_port_config, "--var-json", "PORT"]
        output = self.run_script(argument)
        output_dict = utils.to_dict(output.strip())
        self.assertDictEqual(
            output_dict['Ethernet-IB0'], {
                "lanes": "222",
                "alias": "Recirc0/1",
                "index": "52",
                "role": "Inb",
                "speed": "400000",
                "description": "Recirc0/1",
                "mtu": "9100",
                "tpid": "0x8100",
                "pfc_asym": "off",
                "admin_status": "up"
            })

    def test_minigraph_voq_recirc_ports(self):
        argument = ["-j", self.macsec_profile, "-m", self.sample_graph_voq, "-p", self.voq_port_config, "--var-json", "PORT"]
        output = self.run_script(argument)
        output_dict = utils.to_dict(output.strip())
        self.assertDictEqual(
            output_dict['Ethernet-Rec0'], {
                "lanes": "221",
                "alias": "Recirc0/0",
                "index": "51",
                "role": "Rec",
                "speed": "400000",
                "description": "Recirc0/0",
                "mtu": "9100",
                "tpid": "0x8100",
                "pfc_asym": "off",
                "admin_status": "up"
            })

        argument = ["-j", self.macsec_profile, "-m", self.sample_graph_voq, "-p", self.voq_port_config, "--var-json", "INTERFACE"]
        output = self.run_script(argument)
        output_dict = utils.to_dict(output.strip())
        self.assertDictEqual(
            output_dict['Ethernet-Rec0'],
            {}
        )

    def test_minigraph_dhcp(self):
        argument = ['-m', self.sample_graph_simple_case, '-p', self.port_config, '-v', 'DHCP_RELAY']
        output = self.run_script(argument)
        self.assertEqual(
            utils.to_dict(output.strip()),
            utils.to_dict(
                "{'Vlan1000': {'dhcpv6_servers': ['fc02:2000::1', 'fc02:2000::2']}, "
                "'Vlan2000': {'dhcpv6_servers': ['fc02:2000::3', 'fc02:2000::4']}}"
            )
        )

    def test_minigraph_packet_chassis_acl_local_host(self):
        # CFGGEN_UNIT_TESTING is set to '2' in the set_up function
        # this causes the port_table to have ports from the previous test
        # causing yang validation to fail

        os.environ["CFGGEN_UNIT_TESTING"] = ""
        argument = ['-m', self.packet_chassis_graph, '-v', "ACL_TABLE"]
        output = self.run_script(argument)
        print(output)
        self.assertEqual(
            utils.to_dict(output.strip()),
            utils.to_dict("{'SNMP_ACL': {'policy_desc': 'SNMP_ACL', 'type': 'CTRLPLANE', 'stage': 'ingress', 'services': ['SNMP']}, 'SSH_ONLY': {'policy_desc': 'SSH_ONLY', 'type': 'CTRLPLANE', 'stage': 'ingress', 'services': ['SSH']}}")
        )

        # set it back to the original value
        os.environ["CFGGEN_UNIT_TESTING"] = "2"

    def test_minigraph_packet_chassis_acl_namespace(self):

        argument = ['-m', self.packet_chassis_graph, '-p', self.packet_chassis_port_ini, '-n', "asic1", '-v', "ACL_TABLE"]
        output = self.run_script(argument)
        self.assertEqual(
            utils.to_dict(output.strip()),
            utils.to_dict("{'SNMP_ACL': {'policy_desc': 'SNMP_ACL', 'type': 'CTRLPLANE', 'stage': 'ingress', 'services': ['SNMP']}, 'SSH_ONLY': {'policy_desc': 'SSH_ONLY', 'type': 'CTRLPLANE', 'stage': 'ingress', 'services': ['SSH']}}")
        )

    def test_minigraph_bgp_packet_chassis_peer(self):
        argument = ['-m', self.packet_chassis_graph, '-p', self.packet_chassis_port_ini, '-n', "asic1", '-v', "BGP_INTERNAL_NEIGHBOR[\'8.0.0.1\']"]
        output = self.run_script(argument)
        self.assertEqual(
            utils.to_dict(output.strip()),
            utils.to_dict("{'rrclient': 0, 'name': 'str2-8808-lc0-ASIC1', 'local_addr': '8.0.0.3', 'nhopself': 0, 'admin_status': 'up', 'holdtime': '0', 'asn': '65100', 'keepalive': '0'}")
        )

    def test_minigraph_bgp_packet_chassis_static_route(self):
        argument = ['-m', self.packet_chassis_graph0, '-p', self.packet_chassis_port_ini, '-n', "asic1", '-v', "STATIC_ROUTE"]
        output = self.run_script(argument)
        self.assertEqual(
            utils.to_dict(output.strip()),
            utils.to_dict("{'192.0.0.1/32': {'nexthop': '10.235.96.11,10.235.96.136,10.235.97.11,10.235.97.136,10.235.98.11,10.235.98.136,10.235.99.11,10.235.99.136,10.235.100.11,10.235.100.136', 'ifname': 'PortChannel4001,PortChannel4014,PortChannel4027,PortChannel4040,PortChannel4053,PortChannel4066,PortChannel4079,PortChannel4092,PortChannel4105,PortChannel4118', 'advertise': 'false', 'bfd': 'true'}, '2603:10e2:400::1/128': {'nexthop': 'fc00:34::a:eb60:b01,fc00:34::a:eb60:8801,fc00:34::a:eb61:b01,fc00:34::a:eb61:8801,fc00:34::a:eb62:b01,fc00:34::a:eb62:8801,fc00:34::a:eb63:b01,fc00:34::a:eb63:8801,fc00:34::a:eb64:b01,fc00:34::a:eb64:8801', 'ifname': 'PortChannel4001,PortChannel4014,PortChannel4027,PortChannel4040,PortChannel4053,PortChannel4066,PortChannel4079,PortChannel4092,PortChannel4105,PortChannel4118', 'advertise': 'false', 'bfd': 'true'}, '192.0.0.2/32': {'nexthop': '10.235.96.12,10.235.96.137,10.235.97.12,10.235.97.137,10.235.98.12,10.235.98.137,10.235.99.12,10.235.99.137,10.235.100.12,10.235.100.137', 'ifname': 'PortChannel4001,PortChannel4014,PortChannel4027,PortChannel4040,PortChannel4053,PortChannel4066,PortChannel4079,PortChannel4092,PortChannel4105,PortChannel4118', 'advertise': 'false', 'bfd': 'true'}, '2603:10e2:400::2/128': {'nexthop': 'fc00:34::a:eb60:c01,fc00:34::a:eb60:8901,fc00:34::a:eb61:c01,fc00:34::a:eb61:8901,fc00:34::a:eb62:c01,fc00:34::a:eb62:8901,fc00:34::a:eb63:c01,fc00:34::a:eb63:8901,fc00:34::a:eb64:c01,fc00:34::a:eb64:8901', 'ifname': 'PortChannel4001,PortChannel4014,PortChannel4027,PortChannel4040,PortChannel4053,PortChannel4066,PortChannel4079,PortChannel4092,PortChannel4105,PortChannel4118', 'advertise': 'false', 'bfd': 'true'}, '192.0.0.3/32': {'nexthop': '10.235.96.13,10.235.96.134,10.235.97.13,10.235.97.134,10.235.98.13,10.235.98.134,10.235.99.13,10.235.99.134,10.235.100.13,10.235.100.134', 'ifname': 'PortChannel4001,PortChannel4014,PortChannel4027,PortChannel4040,PortChannel4053,PortChannel4066,PortChannel4079,PortChannel4092,PortChannel4105,PortChannel4118', 'advertise': 'false', 'bfd': 'true'}, '2603:10e2:400::3/128': {'nexthop': 'fc00:34::a:eb60:d01,fc00:34::a:eb60:8601,fc00:34::a:eb61:d01,fc00:34::a:eb61:8601,fc00:34::a:eb62:d01,fc00:34::a:eb62:8601,fc00:34::a:eb63:d01,fc00:34::a:eb63:8601,fc00:34::a:eb64:d01,fc00:34::a:eb64:8601', 'ifname': 'PortChannel4001,PortChannel4014,PortChannel4027,PortChannel4040,PortChannel4053,PortChannel4066,PortChannel4079,PortChannel4092,PortChannel4105,PortChannel4118', 'advertise': 'false', 'bfd': 'true'}, '192.0.0.4/32': {'nexthop': '10.235.96.9,10.235.96.140,10.235.97.9,10.235.97.140,10.235.98.9,10.235.98.140,10.235.99.9,10.235.99.140,10.235.100.9,10.235.100.140', 'ifname': 'PortChannel4001,PortChannel4014,PortChannel4027,PortChannel4040,PortChannel4053,PortChannel4066,PortChannel4079,PortChannel4092,PortChannel4105,PortChannel4118', 'advertise': 'false', 'bfd': 'true'}, '2603:10e2:400::4/128': {'nexthop': 'fc00:34::a:eb60:901,fc00:34::a:eb60:8c01,fc00:34::a:eb61:901,fc00:34::a:eb61:8c01,fc00:34::a:eb62:901,fc00:34::a:eb62:8c01,fc00:34::a:eb63:901,fc00:34::a:eb63:8c01,fc00:34::a:eb64:901,fc00:34::a:eb64:8c01', 'ifname': 'PortChannel4001,PortChannel4014,PortChannel4027,PortChannel4040,PortChannel4053,PortChannel4066,PortChannel4079,PortChannel4092,PortChannel4105,PortChannel4118', 'advertise': 'false', 'bfd': 'true'}, '192.0.0.5/32': {'nexthop': '10.235.96.10,10.235.96.141,10.235.97.10,10.235.97.141,10.235.98.10,10.235.98.141,10.235.99.10,10.235.99.141,10.235.100.10,10.235.100.141', 'ifname': 'PortChannel4001,PortChannel4014,PortChannel4027,PortChannel4040,PortChannel4053,PortChannel4066,PortChannel4079,PortChannel4092,PortChannel4105,PortChannel4118', 'advertise': 'false', 'bfd': 'true'}, '2603:10e2:400::5/128': {'nexthop': 'fc00:34::a:eb60:a01,fc00:34::a:eb60:8d01,fc00:34::a:eb61:a01,fc00:34::a:eb61:8d01,fc00:34::a:eb62:a01,fc00:34::a:eb62:8d01,fc00:34::a:eb63:a01,fc00:34::a:eb63:8d01,fc00:34::a:eb64:a01,fc00:34::a:eb64:8d01', 'ifname': 'PortChannel4001,PortChannel4014,PortChannel4027,PortChannel4040,PortChannel4053,PortChannel4066,PortChannel4079,PortChannel4092,PortChannel4105,PortChannel4118', 'advertise': 'false', 'bfd': 'true'}, '192.0.0.6/32': {'nexthop': '10.235.96.7,10.235.96.138,10.235.97.7,10.235.97.138,10.235.98.7,10.235.98.138,10.235.99.7,10.235.99.138,10.235.100.7,10.235.100.138', 'ifname': 'PortChannel4001,PortChannel4014,PortChannel4027,PortChannel4040,PortChannel4053,PortChannel4066,PortChannel4079,PortChannel4092,PortChannel4105,PortChannel4118', 'advertise': 'false', 'bfd': 'true'}, '2603:10e2:400::6/128': {'nexthop': 'fc00:34::a:eb60:701,fc00:34::a:eb60:8a01,fc00:34::a:eb61:701,fc00:34::a:eb61:8a01,fc00:34::a:eb62:701,fc00:34::a:eb62:8a01,fc00:34::a:eb63:701,fc00:34::a:eb63:8a01,fc00:34::a:eb64:701,fc00:34::a:eb64:8a01', 'ifname': 'PortChannel4001,PortChannel4014,PortChannel4027,PortChannel4040,PortChannel4053,PortChannel4066,PortChannel4079,PortChannel4092,PortChannel4105,PortChannel4118', 'advertise': 'false', 'bfd': 'true'}, '192.0.0.7/32': {'nexthop': '10.235.96.6,10.235.96.139,10.235.97.6,10.235.97.139,10.235.98.6,10.235.98.139,10.235.99.6,10.235.99.139,10.235.100.6,10.235.100.139', 'ifname': 'PortChannel4001,PortChannel4014,PortChannel4027,PortChannel4040,PortChannel4053,PortChannel4066,PortChannel4079,PortChannel4092,PortChannel4105,PortChannel4118', 'advertise': 'false', 'bfd': 'true'}, '2603:10e2:400::7/128': {'nexthop': 'fc00:34::a:eb60:601,fc00:34::a:eb60:8b01,fc00:34::a:eb61:601,fc00:34::a:eb61:8b01,fc00:34::a:eb62:601,fc00:34::a:eb62:8b01,fc00:34::a:eb63:601,fc00:34::a:eb63:8b01,fc00:34::a:eb64:601,fc00:34::a:eb64:8b01', 'ifname': 'PortChannel4001,PortChannel4014,PortChannel4027,PortChannel4040,PortChannel4053,PortChannel4066,PortChannel4079,PortChannel4092,PortChannel4105,PortChannel4118', 'advertise': 'false', 'bfd': 'true'}, '192.0.0.8/32': {'nexthop': '10.235.96.4,10.235.96.132,10.235.97.4,10.235.97.132,10.235.98.4,10.235.98.132,10.235.99.4,10.235.99.132,10.235.100.4,10.235.100.132', 'ifname': 'PortChannel4001,PortChannel4014,PortChannel4027,PortChannel4040,PortChannel4053,PortChannel4066,PortChannel4079,PortChannel4092,PortChannel4105,PortChannel4118', 'advertise': 'false', 'bfd': 'true'}, '2603:10e2:400::8/128': {'nexthop': 'fc00:34::a:eb60:401,fc00:34::a:eb60:8401,fc00:34::a:eb61:401,fc00:34::a:eb61:8401,fc00:34::a:eb62:401,fc00:34::a:eb62:8401,fc00:34::a:eb63:401,fc00:34::a:eb63:8401,fc00:34::a:eb64:401,fc00:34::a:eb64:8401', 'ifname': 'PortChannel4001,PortChannel4014,PortChannel4027,PortChannel4040,PortChannel4053,PortChannel4066,PortChannel4079,PortChannel4092,PortChannel4105,PortChannel4118', 'advertise': 'false', 'bfd': 'true'}, '192.0.0.9/32': {'nexthop': '10.235.96.5,10.235.96.133,10.235.97.5,10.235.97.133,10.235.98.5,10.235.98.133,10.235.99.5,10.235.99.133,10.235.100.5,10.235.100.133', 'ifname': 'PortChannel4001,PortChannel4014,PortChannel4027,PortChannel4040,PortChannel4053,PortChannel4066,PortChannel4079,PortChannel4092,PortChannel4105,PortChannel4118', 'advertise': 'false', 'bfd': 'true'}, '2603:10e2:400::9/128': {'nexthop': 'fc00:34::a:eb60:501,fc00:34::a:eb60:8501,fc00:34::a:eb61:501,fc00:34::a:eb61:8501,fc00:34::a:eb62:501,fc00:34::a:eb62:8501,fc00:34::a:eb63:501,fc00:34::a:eb63:8501,fc00:34::a:eb64:501,fc00:34::a:eb64:8501', 'ifname': 'PortChannel4001,PortChannel4014,PortChannel4027,PortChannel4040,PortChannel4053,PortChannel4066,PortChannel4079,PortChannel4092,PortChannel4105,PortChannel4118', 'advertise': 'false', 'bfd': 'true'}, '192.0.0.10/32': {'nexthop': '10.235.96.8,10.235.96.135,10.235.97.8,10.235.97.135,10.235.98.8,10.235.98.135,10.235.99.8,10.235.99.135,10.235.100.8,10.235.100.135', 'ifname': 'PortChannel4001,PortChannel4014,PortChannel4027,PortChannel4040,PortChannel4053,PortChannel4066,PortChannel4079,PortChannel4092,PortChannel4105,PortChannel4118', 'advertise': 'false', 'bfd': 'true'}, '2603:10e2:400::a/128': {'nexthop': 'fc00:34::a:eb60:801,fc00:34::a:eb60:8701,fc00:34::a:eb61:801,fc00:34::a:eb61:8701,fc00:34::a:eb62:801,fc00:34::a:eb62:8701,fc00:34::a:eb63:801,fc00:34::a:eb63:8701,fc00:34::a:eb64:801,fc00:34::a:eb64:8701', 'ifname': 'PortChannel4001,PortChannel4014,PortChannel4027,PortChannel4040,PortChannel4053,PortChannel4066,PortChannel4079,PortChannel4092,PortChannel4105,PortChannel4118', 'advertise': 'false', 'bfd': 'true'}, '192.0.0.12/32': {'nexthop': '10.235.96.2,10.235.96.130,10.235.97.2,10.235.97.130,10.235.98.2,10.235.98.130,10.235.99.2,10.235.99.130,10.235.100.2,10.235.100.130', 'ifname': 'PortChannel4001,PortChannel4014,PortChannel4027,PortChannel4040,PortChannel4053,PortChannel4066,PortChannel4079,PortChannel4092,PortChannel4105,PortChannel4118', 'advertise': 'false', 'bfd': 'true'}, '2603:10e2:400::c/128': {'nexthop': 'fc00:34::a:eb60:201,fc00:34::a:eb60:8201,fc00:34::a:eb61:201,fc00:34::a:eb61:8201,fc00:34::a:eb62:201,fc00:34::a:eb62:8201,fc00:34::a:eb63:201,fc00:34::a:eb63:8201,fc00:34::a:eb64:201,fc00:34::a:eb64:8201', 'ifname': 'PortChannel4001,PortChannel4014,PortChannel4027,PortChannel4040,PortChannel4053,PortChannel4066,PortChannel4079,PortChannel4092,PortChannel4105,PortChannel4118', 'advertise': 'false', 'bfd': 'true'}, '192.0.0.13/32': {'nexthop': '10.235.96.3,10.235.96.131,10.235.97.3,10.235.97.131,10.235.98.3,10.235.98.131,10.235.99.3,10.235.99.131,10.235.100.3,10.235.100.131', 'ifname': 'PortChannel4001,PortChannel4014,PortChannel4027,PortChannel4040,PortChannel4053,PortChannel4066,PortChannel4079,PortChannel4092,PortChannel4105,PortChannel4118', 'advertise': 'false', 'bfd': 'true'}, '2603:10e2:400::d/128': {'nexthop': 'fc00:34::a:eb60:301,fc00:34::a:eb60:8301,fc00:34::a:eb61:301,fc00:34::a:eb61:8301,fc00:34::a:eb62:301,fc00:34::a:eb62:8301,fc00:34::a:eb63:301,fc00:34::a:eb63:8301,fc00:34::a:eb64:301,fc00:34::a:eb64:8301', 'ifname': 'PortChannel4001,PortChannel4014,PortChannel4027,PortChannel4040,PortChannel4053,PortChannel4066,PortChannel4079,PortChannel4092,PortChannel4105,PortChannel4118', 'advertise': 'false', 'bfd': 'true'}}"))
        argument = ['-m', self.packet_chassis_graph, '-p', self.packet_chassis_port_ini, '-n', "asic1", '-v', "STATIC_ROUTE"]
        output = self.run_script(argument)
        self.assertEqual(
            utils.to_dict(output.strip()),
            utils.to_dict("{'8.0.0.1/32': {'nexthop': '192.168.1.2,192.168.2.2', 'ifname': 'PortChannel40,PortChannel50', 'advertise':'false', 'bfd':'true'}}")
        )
        os.environ["CFGGEN_UNIT_TESTING_TOPOLOGY"] = ""
    def test_minigraph_bgp_packet_chassis_vlan_subintf(self):
        argument = ['-m', self.packet_chassis_graph, '-p', self.packet_chassis_port_ini, '-n', "asic1", '-v', "VLAN_SUB_INTERFACE"]
        output = self.run_script(argument)
        self.assertEqual(
            utils.to_dict(output.strip()),
            utils.to_dict("{('PortChannel32.2', '192.168.1.4/24'): {}, 'PortChannel32.2': {'admin_status': 'up'}, ('PortChannel33.2', '192.168.2.4/24'): {}, 'PortChannel33.2': {'admin_status': 'up'}}")
        )

    def test_minigraph_voq_400g_zr_port_config(self):
        argument = ["-j", self.macsec_profile, "-m", self.sample_graph_voq, "-p", self.voq_port_config, "-v" "PORT[\'Ethernet4\']"]
        output = self.run_script(argument)
        output_dict = utils.to_dict(output.strip())
        self.assertEqual(output_dict['tx_power'], '-10')
        self.assertEqual(output_dict['laser_freq'], 195875)

    def test_minigraph_packet_chassis_400g_zr_port_config(self):
        argument = ["-m", self.packet_chassis_graph, "-p", self.packet_chassis_port_ini, "-n", "asic1", "-v", "PORT[\'Ethernet13\']"]
        output = self.run_script(argument)
        output_dict = utils.to_dict(output.strip())
        self.assertEqual(output_dict['tx_power'], '7.5')
        self.assertEqual(output_dict['laser_freq'], 131000)

    def test_minigraph_400g_to_100G_speed(self):
        argument = ["-j", self.macsec_profile, "-m", self.voq_sample_masic_graph, "-p", self.voq_port_config_400g, "-n",  "asic0", "-v", "PORT"]
        output = self.run_script(argument)
        self.assertEqual(
            utils.to_dict(output.strip()),
            utils.to_dict(
                "{'Ethernet0': {'lanes': '72,73,74,75', 'alias': 'Ethernet1/1', 'index': '1', 'role': 'Ext', 'speed': '100000', 'asic_port_name': 'Eth0-ASIC0', 'fec': 'rs', 'description': 'ARISTA01T3:Ethernet1', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, "
                "'Ethernet8': {'lanes': '80,81,82,83', 'alias': 'Ethernet2/1', 'index': '2', 'role': 'Ext', 'speed': '100000', 'asic_port_name': 'Eth8-ASIC0', 'fec': 'rs', 'description': 'ARISTA01T3:Ethernet2', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, "
                "'Ethernet16': {'lanes': '88,89,90,91', 'alias': 'Ethernet3/1', 'index': '3', 'role': 'Ext', 'speed': '100000', 'asic_port_name': 'Eth16-ASIC0', 'fec': 'rs', 'description': 'ARISTA03T3:Ethernet1', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, "
                "'Ethernet24': {'lanes': '96,97,98,99,100,101,102,103', 'alias': 'Ethernet4/1', 'index': '4', 'role': 'Ext', 'speed': '400000', 'asic_port_name': 'Eth24-ASIC0', 'description': 'ARISTA03T3:Ethernet2', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, "
                "'Ethernet32': {'lanes': '104,105,106,107,108,109,110,111', 'alias': 'Ethernet5/1', 'index': '5', 'role': 'Ext', 'speed': '400000', 'asic_port_name': 'Eth32-ASIC0', 'description': 'ARISTA05T3:Ethernet1', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, "
                "'Ethernet40': {'lanes': '112,113,114,115,116,117,118,119', 'alias': 'Ethernet6/1', 'index': '6', 'role': 'Ext', 'speed': '400000', 'asic_port_name': 'Eth40-ASIC0', 'description': 'ARISTA05T3:Ethernet2', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, "
                "'Ethernet48': {'lanes': '120,121,122,123,124,125,126,127', 'alias': 'Ethernet7/1', 'index': '7', 'role': 'Ext', 'speed': '400000', 'asic_port_name': 'Eth48-ASIC0', 'description': 'ARISTA07T3:Ethernet1', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, "
                "'Ethernet56': {'lanes': '128,129,130,131', 'alias': 'Ethernet8/1', 'index': '8', 'role': 'Ext', 'speed': '100000', 'asic_port_name': 'Eth56-ASIC0', 'fec': 'rs', 'description': 'ARISTA07T3:Ethernet2', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, "
                "'Ethernet64': {'lanes': '136,137,138,139', 'alias': 'Ethernet9/1', 'index': '9', 'role': 'Ext', 'speed': '100000', 'asic_port_name': 'Eth64-ASIC0', 'fec': 'rs', 'description': 'ARISTA09T3:Ethernet1', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, "
                "'Ethernet72': {'lanes': '64,65,66,67', 'alias': 'Ethernet10/1', 'index': '10', 'role': 'Ext', 'speed': '100000', 'asic_port_name': 'Eth72-ASIC0', 'fec': 'rs', 'description': 'ARISTA09T3:Ethernet2', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, "
                "'Ethernet80': {'lanes': '56,57,58,59', 'alias': 'Ethernet11/1', 'index': '11', 'role': 'Ext', 'speed': '100000', 'asic_port_name': 'Eth80-ASIC0', 'fec': 'rs', 'description': 'ARISTA11T3:Ethernet1', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, "
                "'Ethernet88': {'lanes': '48,49,50,51', 'alias': 'Ethernet12/1', 'index': '12', 'role': 'Ext', 'speed': '100000', 'asic_port_name': 'Eth88-ASIC0', 'fec': 'rs', 'description': 'ARISTA11T3:Ethernet2', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, "
                "'Ethernet96': {'lanes': '40,41,42,43', 'alias': 'Ethernet13/1', 'index': '13', 'role': 'Ext', 'speed': '100000', 'asic_port_name': 'Eth96-ASIC0', 'fec': 'rs', 'description': 'ARISTA13T3:Ethernet1', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, "
                "'Ethernet104': {'lanes': '32,33,34,35', 'alias': 'Ethernet14/1', 'index': '14', 'role': 'Ext', 'speed': '100000', 'asic_port_name': 'Eth104-ASIC0', 'fec': 'rs', 'description': 'ARISTA15T3:Ethernet1', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, "
                "'Ethernet112': {'lanes': '24,25,26,27', 'alias': 'Ethernet15/1', 'index': '15', 'role': 'Ext', 'speed': '100000', 'asic_port_name': 'Eth112-ASIC0', 'fec': 'rs', 'description': 'ARISTA15T3:Ethernet2', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, "
                "'Ethernet120': {'lanes': '16,17,18,19', 'alias': 'Ethernet16/1', 'index': '16', 'role': 'Ext', 'speed': '100000', 'asic_port_name': 'Eth120-ASIC0', 'fec': 'rs', 'description': 'ARISTA17T3:Ethernet1', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, "
                "'Ethernet128': {'lanes': '8,9,10,11', 'alias': 'Ethernet17/1', 'index': '17', 'role': 'Ext', 'speed': '100000', 'asic_port_name': 'Eth128-ASIC0', 'fec': 'rs', 'description': 'ARISTA18T3:Ethernet1', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, "
                "'Ethernet136': {'lanes': '0,1,2,3', 'alias': 'Ethernet18/1', 'index': '18', 'role': 'Ext', 'speed': '100000', 'asic_port_name': 'Eth136-ASIC0', 'fec': 'rs', 'description': 'ARISTA18T3:Ethernet2', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, "
                "'Ethernet-Rec0': {'lanes': '221', 'alias': 'Recirc0/0', 'index': '37', 'role': 'Rec', 'speed': '400000', 'asic_port_name': 'Rcy0-ASIC0', 'description': 'Recirc0/0', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'},"
                "'Ethernet-IB0': {'lanes': '222', 'alias': 'Recirc0/1', 'index': '38', 'role': 'Inb', 'speed': '400000', 'asic_port_name': 'Rcy1-ASIC0', 'description': 'Recirc0/1', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}}"
            )
        )

    def test_minigraph_cisco_400g_to_100G_speed(self):
        argument = ["-m", self.sample_cisco_100_graph, "-p", self.sample_cisco_port_config_400g, "-v", "PORT"]
        self.assertTrue(self.yang.validate(argument))
        output = self.run_script(argument)
        self.assertEqual(
            utils.to_dict(output.strip()),
            utils.to_dict(
                "{'Ethernet0': {'lanes': '2304,2305,2306,2307', 'alias': 'etp0a', 'index': '0', 'speed': '100000', 'fec': 'rs', 'description': 'ARISTA01T0:Ethernet1', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, 'Ethernet4': {'lanes': '2308,2309,2310,2311', 'alias': 'etp0b', 'index': '0', 'speed': '100000', 'fec': 'rs', 'description': 'ARISTA02T0:Ethernet1', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, 'Ethernet8': {'lanes': '2320,2321,2322,2323', 'alias': 'etp1a', 'index': '1', 'speed': '100000', 'fec': 'rs', 'description': 'etp1a', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off'}, 'Ethernet12': {'lanes': '2324,2325,2326,2327', 'alias': 'etp1b', 'index': '1', 'speed': '100000', 'fec': 'rs', 'description': 'etp1b', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off'}, 'Ethernet16': {'lanes': '2312,2313,2314,2315', 'alias': 'etp2a', 'index': '2', 'speed': '100000', 'fec': 'rs', 'description': 'ARISTA03T0:Ethernet1', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, 'Ethernet20': {'lanes': '2316,2317,2318,2319', 'alias': 'etp2b', 'index': '2', 'speed': '100000', 'fec': 'rs', 'description': 'ARISTA04T0:Ethernet1', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, 'Ethernet24': {'lanes': '2056,2057,2058,2059', 'alias': 'etp3a', 'index': '3', 'speed': '100000', 'fec': 'rs', 'description': 'etp3a', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off'}, 'Ethernet28': {'lanes': '2060,2061,2062,2063', 'alias': 'etp3b', 'index': '3', 'speed': '100000', 'fec': 'rs', 'description': 'etp3b', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off'}, 'Ethernet32': {'lanes': '1792,1793,1794,1795', 'alias': 'etp4a', 'index': '4', 'speed': '100000', 'fec': 'rs', 'description': 'etp4a', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off'}, 'Ethernet36': {'lanes': '1796,1797,1798,1799', 'alias': 'etp4b', 'index': '4', 'speed': '100000', 'fec': 'rs', 'description': 'etp4b', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off'}, 'Ethernet40': {'lanes': '2048,2049,2050,2051', 'alias': 'etp5a', 'index': '5', 'speed': '100000', 'fec': 'rs', 'description': 'etp5a', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off'}, 'Ethernet44': {'lanes': '2052,2053,2054,2055', 'alias': 'etp5b', 'index': '5', 'speed': '100000', 'fec': 'rs', 'description': 'etp5b', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off'}, 'Ethernet48': {'lanes': '2560,2561,2562,2563', 'alias': 'etp6a', 'index': '6', 'speed': '100000', 'fec': 'rs', 'description': 'etp6a', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off'}, 'Ethernet52': {'lanes': '2564,2565,2566,2567', 'alias': 'etp6b', 'index': '6', 'speed': '100000', 'fec': 'rs', 'description': 'etp6b', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off'}, 'Ethernet56': {'lanes': '2824,2825,2826,2827', 'alias': 'etp7a', 'index': '7', 'speed': '100000', 'fec': 'rs', 'description': 'etp7a', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off'}, 'Ethernet60': {'lanes': '2828,2829,2830,2831', 'alias': 'etp7b', 'index': '7', 'speed': '100000', 'fec': 'rs', 'description': 'etp7b', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off'}, 'Ethernet64': {'lanes': '2832,2833,2834,2835', 'alias': 'etp8a', 'index': '8', 'speed': '100000', 'fec': 'rs', 'description': 'ARISTA05T0:Ethernet1', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, 'Ethernet68': {'lanes': '2836,2837,2838,2839', 'alias': 'etp8b', 'index': '8', 'speed': '100000', 'fec': 'rs', 'description': 'ARISTA06T0:Ethernet1', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, 'Ethernet72': {'lanes': '2816,2817,2818,2819', 'alias': 'etp9a', 'index': '9', 'speed': '100000', 'fec': 'rs', 'description': 'etp9a', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off'}, 'Ethernet76': {'lanes': '2820,2821,2822,2823', 'alias': 'etp9b', 'index': '9', 'speed': '100000', 'fec': 'rs', 'description': 'etp9b', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off'}, 'Ethernet80': {'lanes': '2568,2569,2570,2571', 'alias': 'etp10a', 'index': '10', 'speed': '100000', 'fec': 'rs', 'description': 'ARISTA07T0:Ethernet1', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, 'Ethernet84': {'lanes': '2572,2573,2574,2575', 'alias': 'etp10b', 'index': '10', 'speed': '100000', 'fec': 'rs', 'description': 'ARISTA08T0:Ethernet1', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, 'Ethernet88': {'lanes': '2576,2577,2578,2579', 'alias': 'etp11a', 'index': '11', 'speed': '100000', 'fec': 'rs', 'description': 'etp11a', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off'}, 'Ethernet92': {'lanes': '2580,2581,2582,2583', 'alias': 'etp11b', 'index': '11', 'speed': '100000', 'fec': 'rs', 'description': 'etp11b', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off'}, 'Ethernet96': {'lanes': '1536,1537,1538,1539', 'alias': 'etp12', 'index': '12', 'speed': '100000', 'fec': 'rs', 'description': 'ARISTA01T2:Ethernet1', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, 'Ethernet104': {'lanes': '1800,1801,1802,1803', 'alias': 'etp13', 'index': '13', 'speed': '100000', 'fec': 'rs', 'description': 'ARISTA01T2:Ethernet2', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, 'Ethernet112': {'lanes': '1552,1553,1554,1555', 'alias': 'etp14', 'index': '14', 'speed': '100000', 'fec': 'rs', 'description': 'ARISTA03T2:Ethernet1', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, 'Ethernet120': {'lanes': '1544,1545,1546,1547', 'alias': 'etp15', 'index': '15', 'speed': '100000', 'fec': 'rs', 'description': 'ARISTA03T2:Ethernet2', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, 'Ethernet128': {'lanes': '1296,1297,1298,1299', 'alias': 'etp16', 'index': '16', 'speed': '100000', 'fec': 'rs', 'description': 'ARISTA05T2:Ethernet1', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, 'Ethernet136': {'lanes': '1288,1289,1290,1291', 'alias': 'etp17', 'index': '17', 'speed': '100000', 'fec': 'rs', 'description': 'ARISTA05T2:Ethernet2', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, 'Ethernet144': {'lanes': '1280,1281,1282,1283', 'alias': 'etp18', 'index': '18', 'speed': '100000', 'fec': 'rs', 'description': 'ARISTA07T2:Ethernet1', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, 'Ethernet152': {'lanes': '1032,1033,1034,1035', 'alias': 'etp19', 'index': '19', 'speed': '100000', 'fec': 'rs', 'description': 'ARISTA07T2:Ethernet2', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, 'Ethernet160': {'lanes': '264,265,266,267', 'alias': 'etp20a', 'index': '20', 'speed': '100000', 'fec': 'rs', 'description': 'etp20a', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off'}, 'Ethernet164': {'lanes': '268,269,270,271', 'alias': 'etp20b', 'index': '20', 'speed': '100000', 'fec': 'rs', 'description': 'etp20b', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off'}, 'Ethernet168': {'lanes': '272,273,274,275', 'alias': 'etp21a', 'index': '21', 'speed': '100000', 'fec': 'rs', 'description': 'ARISTA09T0:Ethernet1', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, 'Ethernet172': {'lanes': '276,277,278,279', 'alias': 'etp21b', 'index': '21', 'speed': '100000', 'fec': 'rs', 'description': 'etp21b', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off'}, 'Ethernet176': {'lanes': '16,17,18,19', 'alias': 'etp22a', 'index': '22', 'speed': '100000', 'fec': 'rs', 'description': 'etp22a', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off'}, 'Ethernet180': {'lanes': '20,21,22,23', 'alias': 'etp22b', 'index': '22', 'speed': '100000', 'fec': 'rs', 'description': 'ARISTA10T0:Ethernet1', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, 'Ethernet184': {'lanes': '0,1,2,3', 'alias': 'etp23a', 'index': '23', 'speed': '100000', 'fec': 'rs', 'description': 'etp23a', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off'}, 'Ethernet188': {'lanes': '4,5,6,7', 'alias': 'etp23b', 'index': '23', 'speed': '100000', 'fec': 'rs', 'description': 'etp23b', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off'}, 'Ethernet192': {'lanes': '256,257,258,259', 'alias': 'etp24a', 'index': '24', 'speed': '100000', 'fec': 'rs', 'description': 'etp24a', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off'}, 'Ethernet196': {'lanes': '260,261,262,263', 'alias': 'etp24b', 'index': '24', 'speed': '100000', 'fec': 'rs', 'description': 'etp24b', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off'}, 'Ethernet200': {'lanes': '8,9,10,11', 'alias': 'etp25a', 'index': '25', 'speed': '100000', 'fec': 'rs', 'description': 'ARISTA11T0:Ethernet1', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, 'Ethernet204': {'lanes': '12,13,14,15', 'alias': 'etp25b', 'index': '25', 'speed': '100000', 'fec': 'rs', 'description': 'ARISTA12T0:Ethernet1', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, 'Ethernet208': {'lanes': '1024,1025,1026,1027', 'alias': 'etp26a', 'index': '26', 'speed': '100000', 'fec': 'rs', 'description': 'etp26a', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off'}, 'Ethernet212': {'lanes': '1028,1029,1030,1031', 'alias': 'etp26b', 'index': '26', 'speed': '100000', 'fec': 'rs', 'description': 'ARISTA13T0:Ethernet1', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, 'Ethernet216': {'lanes': '768,769,770,771', 'alias': 'etp27a', 'index': '27', 'speed': '100000', 'fec': 'rs', 'description': 'ARISTA14T0:Ethernet1', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, 'Ethernet220': {'lanes': '772,773,774,775', 'alias': 'etp27b', 'index': '27', 'speed': '100000', 'fec': 'rs', 'description': 'ARISTA15T0:Ethernet1', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, 'Ethernet224': {'lanes': '524,525,526,527', 'alias': 'etp28a', 'index': '28', 'speed': '100000', 'fec': 'rs', 'description': 'etp28a', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off'}, 'Ethernet228': {'lanes': '520,521,522,523', 'alias': 'etp28b', 'index': '28', 'speed': '100000', 'fec': 'rs', 'description': 'etp28b', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off'}, 'Ethernet232': {'lanes': '776,777,778,779', 'alias': 'etp29a', 'index': '29', 'speed': '100000', 'fec': 'rs', 'description': 'ARISTA16T0:Ethernet1', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, 'Ethernet236': {'lanes': '780,781,782,783', 'alias': 'etp29b', 'index': '29', 'speed': '100000', 'fec': 'rs', 'description': 'etp29b', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off'}, 'Ethernet240': {'lanes': '516,517,518,519', 'alias': 'etp30a', 'index': '30', 'speed': '100000', 'fec': 'rs', 'description': 'ARISTA17T0:Ethernet1', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, 'Ethernet244': {'lanes': '512,513,514,515', 'alias': 'etp30b', 'index': '30', 'speed': '100000', 'fec': 'rs', 'description': 'ARISTA18T0:Ethernet1', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, 'Ethernet248': {'lanes': '528,529,530,531', 'alias': 'etp31a', 'index': '31', 'speed': '100000', 'fec': 'rs', 'description': 'ARISTA19T0:Ethernet1', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, 'Ethernet252': {'lanes': '532,533,534,535', 'alias': 'etp31b', 'index': '31', 'speed': '100000', 'fec': 'rs', 'description': 'ARISTA20T0:Ethernet1', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}}"
            )
        )



    def test_minigraph_cisco_400G_to_400G_speed(self):
        argument = ["-m", self.sample_cisco_400_graph, "-p", self.sample_cisco_port_config_400g, "-v", "PORT"]
        output = self.run_script(argument)
        self.assertEqual(
            utils.to_dict(output.strip()),
            utils.to_dict(
                "{'Ethernet0': {'lanes': '2304,2305,2306,2307', 'alias': 'etp0a', 'index': '0', 'speed': '100000', 'fec': 'rs', 'description': 'ARISTA01T0:Ethernet1', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, 'Ethernet4': {'lanes': '2308,2309,2310,2311', 'alias': 'etp0b', 'index': '0', 'speed': '100000', 'fec': 'rs', 'description': 'ARISTA02T0:Ethernet1', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, 'Ethernet8': {'lanes': '2320,2321,2322,2323', 'alias': 'etp1a', 'index': '1', 'speed': '100000', 'fec': 'rs', 'description': 'etp1a', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off'}, 'Ethernet12': {'lanes': '2324,2325,2326,2327', 'alias': 'etp1b', 'index': '1', 'speed': '100000', 'fec': 'rs', 'description': 'etp1b', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off'}, 'Ethernet16': {'lanes': '2312,2313,2314,2315', 'alias': 'etp2a', 'index': '2', 'speed': '100000', 'fec': 'rs', 'description': 'ARISTA03T0:Ethernet1', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, 'Ethernet20': {'lanes': '2316,2317,2318,2319', 'alias': 'etp2b', 'index': '2', 'speed': '100000', 'fec': 'rs', 'description': 'ARISTA04T0:Ethernet1', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, 'Ethernet24': {'lanes': '2056,2057,2058,2059', 'alias': 'etp3a', 'index': '3', 'speed': '100000', 'fec': 'rs', 'description': 'etp3a', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off'}, 'Ethernet28': {'lanes': '2060,2061,2062,2063', 'alias': 'etp3b', 'index': '3', 'speed': '100000', 'fec': 'rs', 'description': 'etp3b', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off'}, 'Ethernet32': {'lanes': '1792,1793,1794,1795', 'alias': 'etp4a', 'index': '4', 'speed': '100000', 'fec': 'rs', 'description': 'etp4a', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off'}, 'Ethernet36': {'lanes': '1796,1797,1798,1799', 'alias': 'etp4b', 'index': '4', 'speed': '100000', 'fec': 'rs', 'description': 'etp4b', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off'}, 'Ethernet40': {'lanes': '2048,2049,2050,2051', 'alias': 'etp5a', 'index': '5', 'speed': '100000', 'fec': 'rs', 'description': 'etp5a', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off'}, 'Ethernet44': {'lanes': '2052,2053,2054,2055', 'alias': 'etp5b', 'index': '5', 'speed': '100000', 'fec': 'rs', 'description': 'etp5b', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off'}, 'Ethernet48': {'lanes': '2560,2561,2562,2563', 'alias': 'etp6a', 'index': '6', 'speed': '100000', 'fec': 'rs', 'description': 'etp6a', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off'}, 'Ethernet52': {'lanes': '2564,2565,2566,2567', 'alias': 'etp6b', 'index': '6', 'speed': '100000', 'fec': 'rs', 'description': 'etp6b', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off'}, 'Ethernet56': {'lanes': '2824,2825,2826,2827', 'alias': 'etp7a', 'index': '7', 'speed': '100000', 'fec': 'rs', 'description': 'etp7a', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off'}, 'Ethernet60': {'lanes': '2828,2829,2830,2831', 'alias': 'etp7b', 'index': '7', 'speed': '100000', 'fec': 'rs', 'description': 'etp7b', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off'}, 'Ethernet64': {'lanes': '2832,2833,2834,2835', 'alias': 'etp8a', 'index': '8', 'speed': '100000', 'fec': 'rs', 'description': 'ARISTA05T0:Ethernet1', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, 'Ethernet68': {'lanes': '2836,2837,2838,2839', 'alias': 'etp8b', 'index': '8', 'speed': '100000', 'fec': 'rs', 'description': 'ARISTA06T0:Ethernet1', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, 'Ethernet72': {'lanes': '2816,2817,2818,2819', 'alias': 'etp9a', 'index': '9', 'speed': '100000', 'fec': 'rs', 'description': 'etp9a', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off'}, 'Ethernet76': {'lanes': '2820,2821,2822,2823', 'alias': 'etp9b', 'index': '9', 'speed': '100000', 'fec': 'rs', 'description': 'etp9b', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off'}, 'Ethernet80': {'lanes': '2568,2569,2570,2571', 'alias': 'etp10a', 'index': '10', 'speed': '100000', 'fec': 'rs', 'description': 'ARISTA07T0:Ethernet1', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, 'Ethernet84': {'lanes': '2572,2573,2574,2575', 'alias': 'etp10b', 'index': '10', 'speed': '100000', 'fec': 'rs', 'description': 'ARISTA08T0:Ethernet1', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, 'Ethernet88': {'lanes': '2576,2577,2578,2579', 'alias': 'etp11a', 'index': '11', 'speed': '100000', 'fec': 'rs', 'description': 'etp11a', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off'}, 'Ethernet92': {'lanes': '2580,2581,2582,2583', 'alias': 'etp11b', 'index': '11', 'speed': '100000', 'fec': 'rs', 'description': 'etp11b', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off'}, 'Ethernet96': {'lanes': '1536,1537,1538,1539,1540,1541,1542,1543', 'alias': 'etp12', 'index': '12', 'speed': '400000', 'description': 'ARISTA01T2:Ethernet1', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, 'Ethernet104': {'lanes': '1800,1801,1802,1803,1804,1805,1806,1807', 'alias': 'etp13', 'index': '13', 'speed': '400000', 'description': 'ARISTA01T2:Ethernet2', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, 'Ethernet112': {'lanes': '1552,1553,1554,1555,1556,1557,1558,1559', 'alias': 'etp14', 'index': '14', 'speed': '400000', 'description': 'ARISTA03T2:Ethernet1', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, 'Ethernet120': {'lanes': '1544,1545,1546,1547,1548,1549,1550,1551', 'alias': 'etp15', 'index': '15', 'speed': '400000', 'description': 'ARISTA03T2:Ethernet2', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, 'Ethernet128': {'lanes': '1296,1297,1298,1299,1300,1301,1302,1303', 'alias': 'etp16', 'index': '16', 'speed': '400000', 'description': 'ARISTA05T2:Ethernet1', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, 'Ethernet136': {'lanes': '1288,1289,1290,1291,1292,1293,1294,1295', 'alias': 'etp17', 'index': '17', 'speed': '400000', 'description': 'ARISTA05T2:Ethernet2', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, 'Ethernet144': {'lanes': '1280,1281,1282,1283,1284,1285,1286,1287', 'alias': 'etp18', 'index': '18', 'speed': '400000', 'description': 'ARISTA07T2:Ethernet1', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, 'Ethernet152': {'lanes': '1032,1033,1034,1035,1036,1037,1038,1039', 'alias': 'etp19', 'index': '19', 'speed': '400000', 'description': 'ARISTA07T2:Ethernet2', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, 'Ethernet160': {'lanes': '264,265,266,267', 'alias': 'etp20a', 'index': '20', 'speed': '100000', 'fec': 'rs', 'description': 'etp20a', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off'}, 'Ethernet164': {'lanes': '268,269,270,271', 'alias': 'etp20b', 'index': '20', 'speed': '100000', 'fec': 'rs', 'description': 'etp20b', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off'}, 'Ethernet168': {'lanes': '272,273,274,275', 'alias': 'etp21a', 'index': '21', 'speed': '100000', 'fec': 'rs', 'description': 'ARISTA09T0:Ethernet1', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, 'Ethernet172': {'lanes': '276,277,278,279', 'alias': 'etp21b', 'index': '21', 'speed': '100000', 'fec': 'rs', 'description': 'etp21b', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off'}, 'Ethernet176': {'lanes': '16,17,18,19', 'alias': 'etp22a', 'index': '22', 'speed': '100000', 'fec': 'rs', 'description': 'etp22a', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off'}, 'Ethernet180': {'lanes': '20,21,22,23', 'alias': 'etp22b', 'index': '22', 'speed': '100000', 'fec': 'rs', 'description': 'ARISTA10T0:Ethernet1', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, 'Ethernet184': {'lanes': '0,1,2,3', 'alias': 'etp23a', 'index': '23', 'speed': '100000', 'fec': 'rs', 'description': 'etp23a', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off'}, 'Ethernet188': {'lanes': '4,5,6,7', 'alias': 'etp23b', 'index': '23', 'speed': '100000', 'fec': 'rs', 'description': 'etp23b', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off'}, 'Ethernet192': {'lanes': '256,257,258,259', 'alias': 'etp24a', 'index': '24', 'speed': '100000', 'fec': 'rs', 'description': 'etp24a', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off'}, 'Ethernet196': {'lanes': '260,261,262,263', 'alias': 'etp24b', 'index': '24', 'speed': '100000', 'fec': 'rs', 'description': 'etp24b', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off'}, 'Ethernet200': {'lanes': '8,9,10,11', 'alias': 'etp25a', 'index': '25', 'speed': '100000', 'fec': 'rs', 'description': 'ARISTA11T0:Ethernet1', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, 'Ethernet204': {'lanes': '12,13,14,15', 'alias': 'etp25b', 'index': '25', 'speed': '100000', 'fec': 'rs', 'description': 'ARISTA12T0:Ethernet1', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, 'Ethernet208': {'lanes': '1024,1025,1026,1027', 'alias': 'etp26a', 'index': '26', 'speed': '100000', 'fec': 'rs', 'description': 'etp26a', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off'}, 'Ethernet212': {'lanes': '1028,1029,1030,1031', 'alias': 'etp26b', 'index': '26', 'speed': '100000', 'fec': 'rs', 'description': 'ARISTA13T0:Ethernet1', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, 'Ethernet216': {'lanes': '768,769,770,771', 'alias': 'etp27a', 'index': '27', 'speed': '100000', 'fec': 'rs', 'description': 'ARISTA14T0:Ethernet1', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, 'Ethernet220': {'lanes': '772,773,774,775', 'alias': 'etp27b', 'index': '27', 'speed': '100000', 'fec': 'rs', 'description': 'ARISTA15T0:Ethernet1', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, 'Ethernet224': {'lanes': '524,525,526,527', 'alias': 'etp28a', 'index': '28', 'speed': '100000', 'fec': 'rs', 'description': 'etp28a', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off'}, 'Ethernet228': {'lanes': '520,521,522,523', 'alias': 'etp28b', 'index': '28', 'speed': '100000', 'fec': 'rs', 'description': 'etp28b', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off'}, 'Ethernet232': {'lanes': '776,777,778,779', 'alias': 'etp29a', 'index': '29', 'speed': '100000', 'fec': 'rs', 'description': 'ARISTA16T0:Ethernet1', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, 'Ethernet236': {'lanes': '780,781,782,783', 'alias': 'etp29b', 'index': '29', 'speed': '100000', 'fec': 'rs', 'description': 'etp29b', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off'}, 'Ethernet240': {'lanes': '516,517,518,519', 'alias': 'etp30a', 'index': '30', 'speed': '100000', 'fec': 'rs', 'description': 'ARISTA17T0:Ethernet1', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, 'Ethernet244': {'lanes': '512,513,514,515', 'alias': 'etp30b', 'index': '30', 'speed': '100000', 'fec': 'rs', 'description': 'ARISTA18T0:Ethernet1', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, 'Ethernet248': {'lanes': '528,529,530,531', 'alias': 'etp31a', 'index': '31', 'speed': '100000', 'fec': 'rs', 'description': 'ARISTA19T0:Ethernet1', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, 'Ethernet252': {'lanes': '532,533,534,535', 'alias': 'etp31b', 'index': '31', 'speed': '100000', 'fec': 'rs', 'description': 'ARISTA20T0:Ethernet1', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}}"
            )
        )


    def test_minigraph_cisco_400g_to_100G_speed_no_lane_change(self):
        argument = ["-m", self.sample_cisco_8111_graph, "-p", self.sample_cisco_8111_port_config, "-v", "PORT"]
        self.assertTrue(self.yang.validate(argument))
        output = self.run_script(argument)
        self.assertEqual(
            utils.to_dict(output.strip()),
            utils.to_dict(
                "{'Ethernet0': {'lanes': '2304,2305,2306,2307', 'alias': 'etp0a', 'index': '0', 'speed': '100000', 'fec': 'rs', 'description': 'ARISTA01T0:Ethernet1', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, 'Ethernet4': {'lanes': '2308,2309,2310,2311', 'alias': 'etp0b', 'index': '0', 'speed': '100000', 'fec': 'rs', 'description': 'ARISTA02T0:Ethernet1', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, 'Ethernet8': {'lanes': '2320,2321,2322,2323', 'alias': 'etp1a', 'index': '1', 'speed': '100000', 'fec': 'rs', 'description': 'etp1a', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off'}, 'Ethernet12': {'lanes': '2324,2325,2326,2327', 'alias': 'etp1b', 'index': '1', 'speed': '100000', 'fec': 'rs', 'description': 'etp1b', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off'}, 'Ethernet16': {'lanes': '2312,2313,2314,2315', 'alias': 'etp2a', 'index': '2', 'speed': '100000', 'fec': 'rs', 'description': 'ARISTA03T0:Ethernet1', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, 'Ethernet20': {'lanes': '2316,2317,2318,2319', 'alias': 'etp2b', 'index': '2', 'speed': '100000', 'fec': 'rs', 'description': 'ARISTA04T0:Ethernet1', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, 'Ethernet24': {'lanes': '2056,2057,2058,2059', 'alias': 'etp3a', 'index': '3', 'speed': '100000', 'fec': 'rs', 'description': 'etp3a', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off'}, 'Ethernet28': {'lanes': '2060,2061,2062,2063', 'alias': 'etp3b', 'index': '3', 'speed': '100000', 'fec': 'rs', 'description': 'etp3b', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off'}, 'Ethernet32': {'lanes': '1792,1793,1794,1795', 'alias': 'etp4a', 'index': '4', 'speed': '100000', 'fec': 'rs', 'description': 'etp4a', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off'}, 'Ethernet36': {'lanes': '1796,1797,1798,1799', 'alias': 'etp4b', 'index': '4', 'speed': '100000', 'fec': 'rs', 'description': 'etp4b', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off'}, 'Ethernet40': {'lanes': '2048,2049,2050,2051', 'alias': 'etp5a', 'index': '5', 'speed': '100000', 'fec': 'rs', 'description': 'etp5a', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off'}, 'Ethernet44': {'lanes': '2052,2053,2054,2055', 'alias': 'etp5b', 'index': '5', 'speed': '100000', 'fec': 'rs', 'description': 'etp5b', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off'}, 'Ethernet48': {'lanes': '2560,2561,2562,2563', 'alias': 'etp6a', 'index': '6', 'speed': '100000', 'fec': 'rs', 'description': 'etp6a', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off'}, 'Ethernet52': {'lanes': '2564,2565,2566,2567', 'alias': 'etp6b', 'index': '6', 'speed': '100000', 'fec': 'rs', 'description': 'etp6b', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off'}, 'Ethernet56': {'lanes': '2824,2825,2826,2827', 'alias': 'etp7a', 'index': '7', 'speed': '100000', 'fec': 'rs', 'description': 'etp7a', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off'}, 'Ethernet60': {'lanes': '2828,2829,2830,2831', 'alias': 'etp7b', 'index': '7', 'speed': '100000', 'fec': 'rs', 'description': 'etp7b', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off'}, 'Ethernet64': {'lanes': '2832,2833,2834,2835', 'alias': 'etp8a', 'index': '8', 'speed': '100000', 'fec': 'rs', 'description': 'ARISTA05T0:Ethernet1', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, 'Ethernet68': {'lanes': '2836,2837,2838,2839', 'alias': 'etp8b', 'index': '8', 'speed': '100000', 'fec': 'rs', 'description': 'ARISTA06T0:Ethernet1', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, 'Ethernet72': {'lanes': '2816,2817,2818,2819', 'alias': 'etp9a', 'index': '9', 'speed': '100000', 'fec': 'rs', 'description': 'etp9a', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off'}, 'Ethernet76': {'lanes': '2820,2821,2822,2823', 'alias': 'etp9b', 'index': '9', 'speed': '100000', 'fec': 'rs', 'description': 'etp9b', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off'}, 'Ethernet80': {'lanes': '2568,2569,2570,2571', 'alias': 'etp10a', 'index': '10', 'speed': '100000', 'fec': 'rs', 'description': 'ARISTA07T0:Ethernet1', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, 'Ethernet84': {'lanes': '2572,2573,2574,2575', 'alias': 'etp10b', 'index': '10', 'speed': '100000', 'fec': 'rs', 'description': 'ARISTA08T0:Ethernet1', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, 'Ethernet88': {'lanes': '2576,2577,2578,2579', 'alias': 'etp11a', 'index': '11', 'speed': '100000', 'fec': 'rs', 'description': 'etp11a', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off'}, 'Ethernet92': {'lanes': '2580,2581,2582,2583', 'alias': 'etp11b', 'index': '11', 'speed': '100000', 'fec': 'rs', 'description': 'etp11b', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off'}, 'Ethernet96': {'lanes': '1536,1537,1538,1539', 'alias': 'etp12', 'index': '12', 'speed': '100000', 'fec': 'rs', 'description': 'ARISTA01T2:Ethernet1', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, 'Ethernet104': {'lanes': '1800,1801,1802,1803', 'alias': 'etp13', 'index': '13', 'speed': '100000', 'fec': 'rs', 'description': 'ARISTA01T2:Ethernet2', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, 'Ethernet112': {'lanes': '1552,1553,1554,1555', 'alias': 'etp14', 'index': '14', 'speed': '100000', 'fec': 'rs', 'description': 'ARISTA03T2:Ethernet1', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, 'Ethernet120': {'lanes': '1544,1545,1546,1547', 'alias': 'etp15', 'index': '15', 'speed': '100000', 'fec': 'rs', 'description': 'ARISTA03T2:Ethernet2', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, 'Ethernet128': {'lanes': '1296,1297,1298,1299', 'alias': 'etp16', 'index': '16', 'speed': '100000', 'fec': 'rs', 'description': 'ARISTA05T2:Ethernet1', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, 'Ethernet136': {'lanes': '1288,1289,1290,1291', 'alias': 'etp17', 'index': '17', 'speed': '100000', 'fec': 'rs', 'description': 'ARISTA05T2:Ethernet2', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, 'Ethernet144': {'lanes': '1280,1281,1282,1283', 'alias': 'etp18', 'index': '18', 'speed': '100000', 'fec': 'rs', 'description': 'ARISTA07T2:Ethernet1', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, 'Ethernet152': {'lanes': '1032,1033,1034,1035', 'alias': 'etp19', 'index': '19', 'speed': '100000', 'fec': 'rs', 'description': 'ARISTA07T2:Ethernet2', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, 'Ethernet160': {'lanes': '264,265,266,267', 'alias': 'etp20a', 'index': '20', 'speed': '100000', 'fec': 'rs', 'description': 'etp20a', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off'}, 'Ethernet164': {'lanes': '268,269,270,271', 'alias': 'etp20b', 'index': '20', 'speed': '100000', 'fec': 'rs', 'description': 'etp20b', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off'}, 'Ethernet168': {'lanes': '272,273,274,275', 'alias': 'etp21a', 'index': '21', 'speed': '100000', 'fec': 'rs', 'description': 'ARISTA09T0:Ethernet1', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, 'Ethernet172': {'lanes': '276,277,278,279', 'alias': 'etp21b', 'index': '21', 'speed': '100000', 'fec': 'rs', 'description': 'etp21b', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off'}, 'Ethernet176': {'lanes': '16,17,18,19', 'alias': 'etp22a', 'index': '22', 'speed': '100000', 'fec': 'rs', 'description': 'etp22a', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off'}, 'Ethernet180': {'lanes': '20,21,22,23', 'alias': 'etp22b', 'index': '22', 'speed': '100000', 'fec': 'rs', 'description': 'ARISTA10T0:Ethernet1', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, 'Ethernet184': {'lanes': '0,1,2,3', 'alias': 'etp23a', 'index': '23', 'speed': '100000', 'fec': 'rs', 'description': 'etp23a', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off'}, 'Ethernet188': {'lanes': '4,5,6,7', 'alias': 'etp23b', 'index': '23', 'speed': '100000', 'fec': 'rs', 'description': 'etp23b', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off'}, 'Ethernet192': {'lanes': '256,257,258,259', 'alias': 'etp24a', 'index': '24', 'speed': '100000', 'fec': 'rs', 'description': 'etp24a', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off'}, 'Ethernet196': {'lanes': '260,261,262,263', 'alias': 'etp24b', 'index': '24', 'speed': '100000', 'fec': 'rs', 'description': 'etp24b', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off'}, 'Ethernet200': {'lanes': '8,9,10,11', 'alias': 'etp25a', 'index': '25', 'speed': '100000', 'fec': 'rs', 'description': 'ARISTA11T0:Ethernet1', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, 'Ethernet204': {'lanes': '12,13,14,15', 'alias': 'etp25b', 'index': '25', 'speed': '100000', 'fec': 'rs', 'description': 'ARISTA12T0:Ethernet1', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, 'Ethernet208': {'lanes': '1024,1025,1026,1027', 'alias': 'etp26a', 'index': '26', 'speed': '100000', 'fec': 'rs', 'description': 'etp26a', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off'}, 'Ethernet212': {'lanes': '1028,1029,1030,1031', 'alias': 'etp26b', 'index': '26', 'speed': '100000', 'fec': 'rs', 'description': 'ARISTA13T0:Ethernet1', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, 'Ethernet216': {'lanes': '768,769,770,771', 'alias': 'etp27a', 'index': '27', 'speed': '100000', 'fec': 'rs', 'description': 'ARISTA14T0:Ethernet1', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, 'Ethernet220': {'lanes': '772,773,774,775', 'alias': 'etp27b', 'index': '27', 'speed': '100000', 'fec': 'rs', 'description': 'ARISTA15T0:Ethernet1', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, 'Ethernet224': {'lanes': '524,525,526,527', 'alias': 'etp28a', 'index': '28', 'speed': '100000', 'fec': 'rs', 'description': 'etp28a', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off'}, 'Ethernet228': {'lanes': '520,521,522,523', 'alias': 'etp28b', 'index': '28', 'speed': '100000', 'fec': 'rs', 'description': 'etp28b', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off'}, 'Ethernet232': {'lanes': '776,777,778,779', 'alias': 'etp29a', 'index': '29', 'speed': '100000', 'fec': 'rs', 'description': 'ARISTA16T0:Ethernet1', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, 'Ethernet236': {'lanes': '780,781,782,783', 'alias': 'etp29b', 'index': '29', 'speed': '100000', 'fec': 'rs', 'description': 'etp29b', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off'}, 'Ethernet240': {'lanes': '516,517,518,519', 'alias': 'etp30a', 'index': '30', 'speed': '100000', 'fec': 'rs', 'description': 'ARISTA17T0:Ethernet1', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, 'Ethernet244': {'lanes': '512,513,514,515', 'alias': 'etp30b', 'index': '30', 'speed': '100000', 'fec': 'rs', 'description': 'ARISTA18T0:Ethernet1', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, 'Ethernet248': {'lanes': '528,529,530,531', 'alias': 'etp31a', 'index': '31', 'speed': '100000', 'fec': 'rs', 'description': 'ARISTA19T0:Ethernet1', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}, 'Ethernet252': {'lanes': '532,533,534,535', 'alias': 'etp31b', 'index': '31', 'speed': '100000', 'fec': 'rs', 'description': 'ARISTA20T0:Ethernet1', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}}"
            )
        )

    def testsnmp_agent_address_config(self):
        argument = ['-m', self.sample_graph, '-p', self.port_config, '-v', 'SNMP_AGENT_ADDRESS_CONFIG.keys()|list']
        output = self.run_script(argument)
        self.assertEqual(
            utils.liststr_to_dict(output.strip()),
            utils.liststr_to_dict("['192.168.200.15|161|', '100.0.0.6|161|', '100.0.0.7|161|', 'fe80::1%Management0|161|']"))
   
    def test_platform_asic_sensors_config(self):
        os.environ["PLATFORM"] = "x86_64-kvm_x86_64-r0"
        argument = ["-H","--print-data"]
        output = self.run_script(argument, check_stderr=False)
        config_json = utils.to_dict(output.strip())
        os.environ["PLATFORM"] = ""
        self.assertEqual(config_json['ASIC_SENSORS'], utils.to_dict("{'ASIC_SENSORS_POLLER_INTERVAL': {'interval': '10'}, 'ASIC_SENSORS_POLLER_STATUS': {'admin_status': 'enable'}}"))


