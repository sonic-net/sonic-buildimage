import filecmp
import json
import os
import shutil
import subprocess
import unittest
import yaml
import tests.common_utils as utils

from unittest import TestCase
from sonic_py_common.general import getstatusoutput_noshell


SKU = 'multi-npu-01'
ASIC_SKU = 'multi-npu-asic'
NUM_ASIC = 4
HOSTNAME = 'multi_npu_platform_01'
DEVICE_TYPE = 'LeafRouter'

class TestMultiNpuCfgGen(TestCase):

    def setUp(self):
        self.yang = utils.YangWrapper()
        self.test_dir = os.path.dirname(os.path.realpath(__file__))
        self.test_data_dir = os.path.join(self.test_dir,  'multi_npu_data')
        self.script_file = [utils.PYTHON_INTERPRETTER, os.path.join(self.test_dir, '..', 'sonic-cfggen')]
        self.sample_graph = os.path.join(self.test_data_dir, 'sample-minigraph.xml')
        self.sample_graph1 = os.path.join(self.test_data_dir, 'sample-minigraph-noportchannel.xml')
        self.sample_port_config = os.path.join(self.test_data_dir, 'sample_port_config.ini')
        self.port_config = []
        for asic in range(NUM_ASIC):
            self.port_config.append(os.path.join(self.test_data_dir, "sample_port_config-{}.ini".format(asic)))
        self.sample_no_asic_port_config = os.path.join(self.test_data_dir, 'sample_port_config-4.ini')
        self.output_file = os.path.join(self.test_dir, 'output')
        os.environ["CFGGEN_UNIT_TESTING"] = "2"

    def run_script(self, argument, check_stderr=True, output_file=None, validateYang=True):
        print('\n    Running sonic-cfggen ' + ' '.join(argument))
        if validateYang:
            self.assertTrue(self.yang.validate(argument))
        if check_stderr:
            output = subprocess.check_output(self.script_file + argument, stderr=subprocess.STDOUT)
        else:
            output = subprocess.check_output(self.script_file + argument)

        if utils.PY3x:
            output = output.decode()
        if output_file:
            with open(output_file, 'w') as f:
                f.write(output)

        linecount = output.strip().count('\n')
        if linecount <= 0:
            print('    Output: ' + output.strip())
        else:
            print('    Output: ({0} lines, {1} bytes)'.format(linecount + 1, len(output)))
        return output

    def run_diff(self, file1, file2):
        _, output = getstatusoutput_noshell(['diff', '-u', file1, file2])
        return output

    def run_frr_asic_case(self, template, target, asic, port_config):
        template_dir = os.path.join(self.test_dir, '..', '..', '..', 'dockers', 'docker-fpm-frr', "frr")
        conf_template = os.path.join(template_dir, template)
        constants = os.path.join(self.test_dir, '..', '..', '..', 'files', 'image_config', 'constants', 'constants.yml')
        cmd = ['-n', asic, '-m', self.sample_graph, '-p', port_config, '-y', constants, '-t', conf_template, '-T', template_dir]
        self.run_script(cmd, output_file=self.output_file)

        original_filename = os.path.join(self.test_dir, 'sample_output', utils.PYvX_DIR, target)
        r = filecmp.cmp(original_filename, self.output_file)
        diff_output = self.run_diff(original_filename, self.output_file) if not r else ""

        return r, "Diff:\n" + diff_output



    def run_script_for_asic(self,argument,asic, port_config=None):
        cmd = argument + ["-n", "asic{}".format(asic)]
        if port_config:
            cmd = argument + ["-n", "asic{}".format(asic), "-p", port_config]
        output = self.run_script(cmd)
        return output

    def test_dummy_run(self):
        argument = []
        output = self.run_script(argument)
        self.assertEqual(output, '')

    def test_hwsku(self):
        argument = ["-v", "DEVICE_METADATA[\'localhost\'][\'hwsku\']", "-m", self.sample_graph, "-p", self.sample_port_config]
        output = self.run_script(argument)
        self.assertEqual(output.strip(), SKU)
        argument = ["-v", "DEVICE_METADATA[\'localhost\'][\'hwsku\']", "-m", self.sample_graph]
        for asic in range(NUM_ASIC):
            output = self.run_script_for_asic(argument, asic, self.port_config[asic])
            self.assertEqual(output.strip(), SKU)

    def test_print_data(self):
        argument = ["-m", self.sample_graph, "-p", self.sample_port_config, "--print-data"]
        output = self.run_script(argument)
        self.assertGreater(len(output.strip()) , 0)
        argument = ["-m", self.sample_graph, "--print-data"]
        for asic in range(NUM_ASIC):
            output = self.run_script_for_asic(argument, asic, self.port_config[asic])
            self.assertGreater(len(output.strip()) , 0)

    def test_additional_json_data(self):
        argument = ['-a', '{"key1":"value1"}', '-v', 'key1']
        output = self.run_script(argument)
        self.assertEqual(output.strip(), 'value1')
        for asic in range(NUM_ASIC):
            output = self.run_script_for_asic(argument, asic, self.port_config[asic])
            self.assertEqual(output.strip(), 'value1')

    def test_read_yaml(self):
        argument = ['-v', 'yml_item', '-y', os.path.join(self.test_dir, 'test.yml')]
        output = yaml.safe_load(self.run_script(argument))
        self.assertListEqual(output, ['value1', 'value2'])
        for asic in range(NUM_ASIC):
            output = yaml.safe_load(self.run_script_for_asic(argument, asic, self.port_config[asic]))
            self.assertListEqual(output, ['value1', 'value2'])

    def test_render_template(self):
        argument = ['-y', os.path.join(self.test_dir, 'test.yml'), '-t', os.path.join(self.test_dir, 'test.j2')]
        output = self.run_script(argument)
        self.assertEqual(output.strip(), 'value1\nvalue2')
        for asic in range(NUM_ASIC):
            output = self.run_script_for_asic(argument, asic, self.port_config[asic])
            self.assertEqual(output.strip(), 'value1\nvalue2')

    def test_metadata_tacacs(self):
        argument = ['-m', self.sample_graph, '-p', self.sample_port_config, '--var-json', "TACPLUS_SERVER"]
        output = json.loads(self.run_script(argument))
        self.assertDictEqual(output, {'123.46.98.21': {'priority': '1', 'tcp_port': '49'}})
        #TACPLUS_SERVER not present in the asic configuration.
        argument = ['-m', self.sample_graph, '--var-json', "TACPLUS_SERVER"]
        for asic in range(NUM_ASIC):
            output = json.loads(self.run_script_for_asic(argument, asic, self.port_config[asic]))
            self.assertDictEqual(output, {})

    def test_metadata_ntp(self):
        argument = ['-m', self.sample_graph, '-p', self.sample_port_config, '--var-json', "NTP_SERVER"]
        output = json.loads(self.run_script(argument))
        self.assertDictEqual(output, {'17.39.1.130': {}, '17.39.1.129': {}})
        #NTP data is present only in the host config
        argument = ['-m', self.sample_graph, '--var-json', "NTP_SERVER"]
        for asic in range(NUM_ASIC):
            output = json.loads(self.run_script_for_asic(argument, asic, self.port_config[asic]))
            print("Log:asic{} sku {}".format(asic,output))
            self.assertDictEqual(output, {})

    def test_mgmt_port(self):
        argument = ['-m', self.sample_graph, '-p', self.sample_port_config, '--var-json', "MGMT_PORT"]
        output = json.loads(self.run_script(argument))
        self.assertDictEqual(output, {'eth0': {'alias': 'eth0', 'admin_status': 'up'}})
        argument = ['-m', self.sample_graph, '--var-json', "MGMT_PORT"]
        for asic in range(NUM_ASIC):
            output = json.loads(self.run_script_for_asic(argument, asic, self.port_config[asic]))
            self.assertDictEqual(output, {'eth0': {'alias': 'eth0', 'admin_status': 'up'}})

    def test_frontend_asic_portchannels(self):
        argument = ["-m", self.sample_graph, "-p", self.port_config[0], "-n", "asic0", "--var-json", "PORTCHANNEL"]
        output = json.loads(self.run_script(argument))
        self.assertDictEqual(output, \
                {'PortChannel0002': {'admin_status': 'up', 'min_links': '2', 'mtu': '9100', 'tpid': '0x8100', 'lacp_key': 'auto'},
                 'PortChannel4001': {'admin_status': 'up', 'min_links': '2', 'mtu': '9100', 'tpid': '0x8100', 'lacp_key': 'auto'},
                 'PortChannel4002': {'admin_status': 'up', 'min_links': '2', 'mtu': '9100', 'tpid': '0x8100', 'lacp_key': 'auto'}})

    def test_backend_asic_portchannels(self):
        argument = ["-m", self.sample_graph, "-p", self.port_config[3], "-n", "asic3", "--var-json", "PORTCHANNEL"]
        output = json.loads(self.run_script(argument))
        self.assertDictEqual(output, \
                {'PortChannel4013': {'admin_status': 'up', 'min_links': '2', 'mtu': '9100', 'tpid': '0x8100', 'lacp_key': 'auto'},
                 'PortChannel4014': {'admin_status': 'up', 'min_links': '2', 'mtu': '9100', 'tpid': '0x8100', 'lacp_key': 'auto'}})

    def test_frontend_asic_portchannel_mem(self):
        argument = ["-m", self.sample_graph, "-p", self.port_config[0], "-n", "asic0", "-v", "PORTCHANNEL_MEMBER.keys()|list"]
        output = self.run_script(argument)
        self.assertEqual(
            utils.liststr_to_dict(output.strip()),
            utils.liststr_to_dict("['PortChannel4002|Ethernet-BP8', 'PortChannel0002|Ethernet0', 'PortChannel0002|Ethernet4', 'PortChannel4002|Ethernet-BP12', 'PortChannel4001|Ethernet-BP0', 'PortChannel4001|Ethernet-BP4']")
        )

    def test_backend_asic_portchannels_mem(self):
        argument = ["-m", self.sample_graph, "-p", self.port_config[3], "-n", "asic3", "-v", "PORTCHANNEL_MEMBER.keys()|list"]
        output = self.run_script(argument)
        self.assertEqual(
            utils.liststr_to_dict(output.strip()),
            utils.liststr_to_dict("['PortChannel4013|Ethernet-BP384', 'PortChannel4014|Ethernet-BP392', 'PortChannel4014|Ethernet-BP396', 'PortChannel4013|Ethernet-BP388']")
        )

    def test_frontend_asic_portchannel_intf(self):
        argument = ["-m", self.sample_graph, "-p", self.port_config[0], "-n", "asic0", "-v", "PORTCHANNEL_INTERFACE.keys()|list"]
        output = self.run_script(argument)
        self.assertEqual(
            utils.liststr_to_dict(output.strip()),
            utils.liststr_to_dict("['PortChannel4001|10.1.0.1/31', 'PortChannel0002|FC00::1/126', 'PortChannel4002|10.1.0.3/31', 'PortChannel0002', 'PortChannel0002|10.0.0.0/31', 'PortChannel4001', 'PortChannel4002']")
        )

    def test_frontend_asic_routerport_intf(self):
        argument = ["-m", self.sample_graph1, "-p", self.port_config[0], "-n", "asic0", "-v", "INTERFACE.keys()|list"]
        output = self.run_script(argument)
        self.assertEqual(
            utils.liststr_to_dict(output.strip()),
            utils.liststr_to_dict("['Ethernet0', ('Ethernet0', '10.0.0.0/31'), 'Ethernet4', ('Ethernet0', 'FC00::1/126'), ('Ethernet4', 'FC00::2/126'), ('Ethernet4', '10.0.0.2/31')]")
        )

    def test_backend_asic_portchannel_intf(self):
        argument = ["-m", self.sample_graph, "-p", self.port_config[3], "-n", "asic3", "-v", "PORTCHANNEL_INTERFACE.keys()|list"]
        output = self.run_script(argument)
        self.assertEqual(
            utils.liststr_to_dict(output.strip()),
            utils.liststr_to_dict("['PortChannel4013', 'PortChannel4013|10.1.0.2/31', 'PortChannel4014', 'PortChannel4014|10.1.0.6/31']")
        )

    def test_frontend_asic_ports(self):
        argument = ["-m", self.sample_graph, "-p", self.port_config[0], "-n", "asic0", "--var-json", "PORT"]
        output = json.loads(self.run_script(argument))
        self.assertDictEqual(output,
            {"Ethernet0": { "admin_status": "up",  "alias": "Ethernet1/1",  "asic_port_name": "Eth0-ASIC0",  "description": "01T2:Ethernet1",  "index": "0",  "lanes": "33,34,35,36",  "mtu": "9100", "tpid": "0x8100", "pfc_asym": "off",  "role": "Ext",  "speed": "40000", "autoneg": "on" },
             "Ethernet4": { "admin_status": "up",  "alias": "Ethernet1/2",  "asic_port_name": "Eth1-ASIC0",  "description": "01T2:Ethernet2",  "index": "1",  "lanes": "29,30,31,32",  "mtu": "9100", "tpid": "0x8100", "pfc_asym": "off",  "role": "Ext",  "speed": "40000", "autoneg": "on" },
             "Ethernet8": { "alias": "Ethernet1/3",  "asic_port_name": "Eth2-ASIC0",  "description": "Ethernet1/3",  "index": "2",  "lanes": "41,42,43,44",  "mtu": "9100", "tpid": "0x8100", "pfc_asym": "off",  "role": "Ext",  "speed": "40000" },
             "Ethernet12": { "alias": "Ethernet1/4",  "asic_port_name": "Eth3-ASIC0",  "description": "Ethernet1/4",  "index": "3",  "lanes": "37,38,39,40",  "mtu": "9100", "tpid": "0x8100", "pfc_asym": "off",  "role": "Ext",  "speed": "40000" },
             "Ethernet-BP0": { "admin_status": "up",  "alias": "Eth4-ASIC0",  "asic_port_name": "Eth4-ASIC0",  "description": "ASIC2:Eth0-ASIC2",  "index": "0",  "lanes": "13,14,15,16",  "mtu": "9100", "tpid": "0x8100", "pfc_asym": "off",  "role": "Int",  "speed": "40000" },
             "Ethernet-BP4": { "admin_status": "up",  "alias": "Eth5-ASIC0",  "asic_port_name": "Eth5-ASIC0",  "description": "ASIC2:Eth1-ASIC2",  "index": "1",  "lanes": "17,18,19,20",  "mtu": "9100", "tpid": "0x8100", "pfc_asym": "off",  "role": "Int",  "speed": "40000" },
             "Ethernet-BP8": { "admin_status": "up",  "alias": "Eth6-ASIC0",  "asic_port_name": "Eth6-ASIC0",  "description": "ASIC3:Eth0-ASIC3",  "index": "2",  "lanes": "21,22,23,24",  "mtu": "9100", "tpid": "0x8100", "pfc_asym": "off",  "role": "Int",  "speed": "40000" },
             "Ethernet-BP12": { "admin_status": "up",  "alias": "Eth7-ASIC0",  "asic_port_name": "Eth7-ASIC0",  "description": "ASIC3:Eth1-ASIC3",  "index": "3",  "lanes": "25,26,27,28",  "mtu": "9100", "tpid": "0x8100", "pfc_asym": "off",  "role": "Int",  "speed": "40000" }})

    def test_frontend_asic_ports_config_db(self):
        argument = ["-m", self.sample_graph, "-p", self.port_config[0], "-n", "asic0", "--var-json", "PORT"]
        output = json.loads(self.run_script(argument))
        self.assertDictEqual(output,
                {"Ethernet0": { "admin_status": "up",  "alias": "Ethernet1/1",  "asic_port_name": "Eth0-ASIC0",  "description": "01T2:Ethernet1",  "index": "0",  "lanes": "33,34,35,36",   "mtu": "9100", "tpid": "0x8100",   "pfc_asym": "off",  "role": "Ext",  "speed": "40000", "autoneg": "on" },
                "Ethernet4": { "admin_status": "up",  "alias": "Ethernet1/2",  "asic_port_name": "Eth1-ASIC0",  "description": "01T2:Ethernet2",  "index": "1",  "lanes": "29,30,31,32",   "mtu": "9100", "tpid": "0x8100",  "pfc_asym": "off",  "role": "Ext",  "speed": "40000", "autoneg": "on" },
                "Ethernet8": { "alias": "Ethernet1/3",  "asic_port_name": "Eth2-ASIC0",  "description": "Ethernet1/3",  "index": "2",  "lanes": "41,42,43,44",   "mtu": "9100", "tpid": "0x8100",  "pfc_asym": "off",  "role": "Ext",  "speed": "40000" },
                "Ethernet12": { "alias": "Ethernet1/4",  "asic_port_name": "Eth3-ASIC0",  "description": "Ethernet1/4",  "index": "3",  "lanes": "37,38,39,40",   "mtu": "9100", "tpid": "0x8100",  "pfc_asym": "off",  "role": "Ext",  "speed": "40000" },
                "Ethernet-BP0": { "admin_status": "up",  "alias": "Eth4-ASIC0",  "asic_port_name": "Eth4-ASIC0",  "description": "ASIC2:Eth0-ASIC2",  "index": "0",  "lanes": "13,14,15,16",   "mtu": "9100", "tpid": "0x8100",  "pfc_asym": "off",  "role": "Int",  "speed": "40000" },
                    "Ethernet-BP4": { "admin_status": "up",  "alias": "Eth5-ASIC0",  "asic_port_name": "Eth5-ASIC0",  "description": "ASIC2:Eth1-ASIC2",  "index": "1",  "lanes": "17,18,19,20",   "mtu": "9100", "tpid": "0x8100",  "pfc_asym": "off",  "role": "Int",  "speed": "40000" },
                    "Ethernet-BP8": { "admin_status": "up",  "alias": "Eth6-ASIC0",  "asic_port_name": "Eth6-ASIC0",  "description": "ASIC3:Eth0-ASIC3",  "index": "2",  "lanes": "21,22,23,24",   "mtu": "9100", "tpid": "0x8100",  "pfc_asym": "off",  "role": "Int",  "speed": "40000" },
                    "Ethernet-BP12": { "admin_status": "up",  "alias": "Eth7-ASIC0",  "asic_port_name": "Eth7-ASIC0",  "description": "ASIC3:Eth1-ASIC3",  "index": "3",  "lanes": "25,26,27,28",   "mtu": "9100", "tpid": "0x8100",  "pfc_asym": "off",  "role": "Int",  "speed": "40000" }})

    def test_frontend_asic_device_neigh(self):
        argument = ["-m", self.sample_graph, "-p", self.port_config[0], "-n", "asic0", "--var-json", "DEVICE_NEIGHBOR"]
        output = json.loads(self.run_script(argument))
        self.assertDictEqual(output, \
            {'Ethernet0': {'name': '01T2', 'port': 'Ethernet1'},
            'Ethernet4': {'name': '01T2', 'port': 'Ethernet2'},
            'Ethernet-BP4': {'name': 'ASIC2', 'port': 'Eth1-ASIC2'},
            'Ethernet-BP12': {'name': 'ASIC3', 'port': 'Eth1-ASIC3'},
            'Ethernet-BP0': {'name': 'ASIC2', 'port': 'Eth0-ASIC2'},
            'Ethernet-BP8': {'name': 'ASIC3', 'port': 'Eth0-ASIC3'}})

    def test_frontend_asic_device_neigh_metadata(self):
        argument = ["-m", self.sample_graph, "-p", self.port_config[0], "-n", "asic0", "--var-json", "DEVICE_NEIGHBOR_METADATA"]
        output = json.loads(self.run_script(argument))
        print(output)
        self.assertDictEqual(output, \
            {'01T2': {'mgmt_addr': '89.139.132.40', 'hwsku': 'VM', 'type': 'SpineRouter'},
            'ASIC3': {'lo_addr_v6': '::/0', 'mgmt_addr': '0.0.0.0/0', 'hwsku': 'multi-npu-asic', 'lo_addr': '0.0.0.0/0', 'type': 'Asic', 'mgmt_addr_v6': '::/0'},
            'ASIC2': {'lo_addr_v6': '::/0', 'mgmt_addr': '0.0.0.0/0', 'hwsku': 'multi-npu-asic', 'lo_addr': '0.0.0.0/0', 'type': 'Asic', 'mgmt_addr_v6': '::/0'}})

    def test_backend_asic_device_neigh(self):
        argument = ["-m", self.sample_graph, "-p", self.port_config[3], "-n", "asic3", "--var-json", "DEVICE_NEIGHBOR"]
        output = json.loads(self.run_script(argument))
        self.assertDictEqual(output, \
            {'Ethernet-BP396': {'name': 'ASIC1', 'port': 'Eth7-ASIC1'},
            'Ethernet-BP384': {'name': 'ASIC0', 'port': 'Eth6-ASIC0'},
            'Ethernet-BP392': {'name': 'ASIC1', 'port': 'Eth6-ASIC1'},
            'Ethernet-BP388': {'name': 'ASIC0', 'port': 'Eth7-ASIC0'}})

    def test_backend_device_neigh_metadata(self):
        argument = ["-m", self.sample_graph, "-p", self.port_config[3], "-n", "asic3", "--var-json", "DEVICE_NEIGHBOR_METADATA"]
        output = json.loads(self.run_script(argument))
        print(output)
        self.assertDictEqual(output, \
            {'ASIC1': {'lo_addr_v6': '::/0', 'mgmt_addr': '0.0.0.0/0', 'hwsku': 'multi-npu-asic', 'lo_addr': '0.0.0.0/0', 'type': 'Asic', 'mgmt_addr_v6': '::/0'},
             'ASIC0': {'lo_addr_v6': '::/0', 'mgmt_addr': '0.0.0.0/0', 'hwsku': 'multi-npu-asic', 'lo_addr': '0.0.0.0/0', 'type': 'Asic', 'mgmt_addr_v6': '::/0'}})

    def test_frontend_bgp_neighbor(self):
        argument = ["-m", self.sample_graph, "-p", self.port_config[0], "-n", "asic0", "--var-json", "BGP_NEIGHBOR"]
        output = json.loads(self.run_script(argument))
        self.assertDictEqual(output, \
            {'10.0.0.1': {'rrclient': 0, 'name': '01T2', 'local_addr': '10.0.0.0', 'nhopself': 0, 'holdtime': '10', 'asn': '65200', 'keepalive': '3'},
            'fc00::2': {'rrclient': 0, 'name': '01T2', 'local_addr': 'fc00::1', 'nhopself': 0, 'holdtime': '10', 'asn': '65200', 'keepalive': '3'}})

    def test_frontend_asic_bgp_neighbor(self):
        argument = ["-m", self.sample_graph, "-p", self.port_config[0], "-n", "asic0", "--var-json", "BGP_INTERNAL_NEIGHBOR"]
        output = json.loads(self.run_script(argument))
        self.assertDictEqual(output, \
                {'10.1.0.0': {'rrclient': 0, 'name': 'ASIC2', 'local_addr': '10.1.0.1', 'nhopself': 0, 'admin_status': 'up', 'holdtime': '0', 'asn': '65100', 'keepalive': '0'},
                    '10.1.0.2': {'rrclient': 0, 'name': 'ASIC3', 'local_addr': '10.1.0.3', 'nhopself': 0, 'admin_status': 'up', 'holdtime': '0', 'asn': '65100', 'keepalive': '0'}})

    def test_backend_asic_bgp_neighbor(self):
        argument = ["-m", self.sample_graph, "-p", self.port_config[3], "-n", "asic3", "--var-json", "BGP_INTERNAL_NEIGHBOR"]
        output = json.loads(self.run_script(argument))
        self.assertDictEqual(output, \
                {'10.1.0.7': {'rrclient': 0, 'name': 'ASIC1', 'local_addr': '10.1.0.6', 'nhopself': 0, 'admin_status': 'up', 'holdtime': '0', 'asn': '65100', 'keepalive': '0'},
                    '10.1.0.3': {'rrclient': 0, 'name': 'ASIC0', 'local_addr': '10.1.0.2', 'nhopself': 0, 'admin_status': 'up', 'holdtime': '0', 'asn': '65100', 'keepalive': '0'}})

    def test_device_asic_metadata(self):
        argument = ["-m", self.sample_graph, "--var-json", "DEVICE_METADATA"]
        for asic in range(NUM_ASIC):
            output = json.loads(self.run_script_for_asic(argument, asic,self.port_config[asic]))
            asic_name  = "asic{}".format(asic)
            self.assertEqual(output['localhost']['hostname'], 'multi_npu_platform_01')
            self.assertEqual(output['localhost']['asic_name'], asic_name)
            self.assertEqual(output['localhost']['type'], DEVICE_TYPE)
            if asic == 0 or asic == 1:
                self.assertEqual(output['localhost']['sub_role'], 'FrontEnd')
            else:
                self.assertEqual(output['localhost']['sub_role'], 'BackEnd')
            self.assertEqual(output['localhost']['deployment_id'], "1")

    def test_global_asic_acl(self):
        argument = ["-m", self.sample_graph, "-p", self.sample_port_config, "--var-json", "ACL_TABLE"]
        output = json.loads(self.run_script(argument))
        exp = {\
                'SNMP_ACL': {'policy_desc': 'SNMP_ACL', 'type': 'CTRLPLANE', 'stage': 'ingress', 'services': ['SNMP']},
                'EVERFLOW': {'policy_desc': 'EVERFLOW', 'stage': 'ingress', 'ports': ['PortChannel0002', 'PortChannel0008', 'Ethernet8', 'Ethernet12', 'Ethernet24', 'Ethernet28'], 'type': 'MIRROR'},
                'EVERFLOWV6': {'policy_desc': 'EVERFLOWV6', 'stage': 'ingress', 'ports': ['PortChannel0002', 'PortChannel0008', 'Ethernet8', 'Ethernet12', 'Ethernet24', 'Ethernet28'], 'type': 'MIRRORV6'},
                'SSH_ONLY': {'policy_desc': 'SSH_ONLY', 'type': 'CTRLPLANE', 'stage': 'ingress', 'services': ['SSH']},
                'DATAACL': {'policy_desc': 'DATAACL', 'stage': 'ingress', 'ports': ['PortChannel0002', 'PortChannel0008'], 'type': 'L3'}}
        for k, v in output.items():
            if 'ports' in v:
                v['ports'].sort()
        for k, v in exp.items():
            if 'ports' in v:
                v['ports'].sort()
        self.assertDictEqual(output, exp)

    def test_global_asic_acl1(self):
        argument = ["-m", self.sample_graph1, "-p", self.sample_port_config, "--var-json", "ACL_TABLE"]
        self.maxDiff = None
        output = json.loads(self.run_script(argument))
        exp = {\
                'SNMP_ACL': {'policy_desc': 'SNMP_ACL', 'type': 'CTRLPLANE', 'stage': 'ingress', 'services': ['SNMP']},
                'EVERFLOW': {'policy_desc': 'EVERFLOW', 'stage': 'ingress', 'ports': ['Ethernet0', 'Ethernet4', 'Ethernet8', 'Ethernet12', 'Ethernet16', 'Ethernet20', 'Ethernet24', 'Ethernet28'], 'type': 'MIRROR'},
                'EVERFLOWV6': {'policy_desc': 'EVERFLOWV6', 'stage': 'ingress', 'ports': ['Ethernet0', 'Ethernet4', 'Ethernet8', 'Ethernet12', 'Ethernet16', 'Ethernet20', 'Ethernet24', 'Ethernet28'], 'type': 'MIRRORV6'},
                'SSH_ONLY': {'policy_desc': 'SSH_ONLY', 'type': 'CTRLPLANE', 'stage': 'ingress', 'services': ['SSH']}}
        for k, v in output.items():
            if 'ports' in v:
                v['ports'].sort()
        for k, v in exp.items():
            if 'ports' in v:
                v['ports'].sort()
        self.assertDictEqual(output, exp)

    def test_front_end_asic_acl(self):
        argument = ["-m", self.sample_graph, "-p", self.port_config[0], "-n", "asic0", "--var-json", "ACL_TABLE"]
        output = json.loads(self.run_script(argument))
        self.assertDictEqual(output, {\
                             'DATAACL':   {'policy_desc': 'DATAACL',    'ports': ['PortChannel0002'], 'stage': 'ingress', 'type': 'L3'},
                             'EVERFLOW':  {'policy_desc': 'EVERFLOW',   'ports': ['PortChannel0002'], 'stage': 'ingress', 'type': 'MIRROR'},
                             'EVERFLOWV6':{'policy_desc': 'EVERFLOWV6', 'ports': ['PortChannel0002'], 'stage': 'ingress', 'type': 'MIRRORV6'},
                             'SNMP_ACL':  {'policy_desc': 'SNMP_ACL',    'services': ['SNMP'],        'stage': 'ingress', 'type': 'CTRLPLANE'},
                             'SSH_ONLY':  {'policy_desc': 'SSH_ONLY',    'services': ['SSH'],         'stage': 'ingress', 'type': 'CTRLPLANE'}})

    def test_front_end_asic_acl1(self):
        argument = ["-m", self.sample_graph1, "-p", self.port_config[0], "-n", "asic0", "--var-json", "ACL_TABLE"]
        output = json.loads(self.run_script(argument))
        self.assertDictEqual(output, {\
                             'EVERFLOW':  {'policy_desc': 'EVERFLOW',   'ports': ['Ethernet0','Ethernet4'], 'stage': 'ingress', 'type': 'MIRROR'},
                             'EVERFLOWV6':{'policy_desc': 'EVERFLOWV6', 'ports': ['Ethernet0','Ethernet4'], 'stage': 'ingress', 'type': 'MIRRORV6'},
                             'SNMP_ACL':  {'policy_desc': 'SNMP_ACL',    'services': ['SNMP'],        'stage': 'ingress', 'type': 'CTRLPLANE'},
                             'SSH_ONLY':  {'policy_desc': 'SSH_ONLY',    'services': ['SSH'],         'stage': 'ingress', 'type': 'CTRLPLANE'}})


    def test_back_end_asic_acl(self):
        argument = ["-m", self.sample_graph, "-p", self.port_config[3], "-n", "asic3", "--var-json", "ACL_TABLE"]
        output = json.loads(self.run_script(argument))
        self.assertDictEqual(output, {})

    def test_back_end_asic_acl1(self):
        argument = ["-m", self.sample_graph1, "-p", self.port_config[3], "-n", "asic3", "--var-json", "ACL_TABLE"]
        output = json.loads(self.run_script(argument))
        self.assertDictEqual(output, {})


    def test_loopback_intfs(self):
        argument = ["-m", self.sample_graph, "-p", self.sample_port_config, "--var-json", "LOOPBACK_INTERFACE"]
        output = json.loads(self.run_script(argument))
        self.assertDictEqual(output, {\
                "Loopback0": {},
                "Loopback0|10.1.0.32/32": {},
                "Loopback0|FC00:1::32/128": {}})

        # The asic configuration should have 2 loopback interfaces
        argument = ["-m", self.sample_graph, "-p", self.port_config[0], "-n", "asic0", "--var-json", "LOOPBACK_INTERFACE"]
        output = json.loads(self.run_script(argument))
        self.assertDictEqual(output, { \
                             "Loopback0": {},
                             "Loopback4096": {},
                             "Loopback0|10.1.0.32/32": {},
                             "Loopback0|FC00:1::32/128": {},
                             "Loopback4096|8.0.0.0/32": {},
                             "Loopback4096|FD00:1::32/128": {}})

        argument = ["-m", self.sample_graph, "-p", self.port_config[3], "-n", "asic3", "--var-json", "LOOPBACK_INTERFACE"]
        output = json.loads(self.run_script(argument))
        self.assertDictEqual(output, {\
                                      "Loopback0": {},
                                      "Loopback4096": {},
                                      "Loopback0|10.1.0.32/32": {},
                                      "Loopback0|FC00:1::32/128": {},
                                      "Loopback4096|8.0.0.5/32": {},
                                      "Loopback4096|FD00:4::32/128": {}})

    def test_buffers_multi_asic_template(self):
        build_root_dir = os.path.join(
            self.test_dir, "..", "..", ".."
        )
        # using Trident2 buffer configuration
        device_config_dir = os.path.join(
            build_root_dir,
            "device",
            "arista",
            "x86_64-arista_7050_qx32",
            "Arista-7050-QX32"
        )
        device_buffer_template = os.path.join(
            device_config_dir, "buffers.json.j2"
        )
        buffer_template = os.path.join(
            build_root_dir, "files", "build_templates", "buffers_config.j2"
        )
        port_config_ini_asic0 = os.path.join(
            self.test_data_dir, "sample_port_config-0.ini"
        )
        # asic0 - mix of front end and back end ports
        shutil.copy2(buffer_template, device_config_dir)
        argument = ["-m", self.sample_graph, "-p", port_config_ini_asic0, "-n", "asic0", "-t", device_buffer_template]
        output = json.loads(self.run_script(argument))
        os.remove(os.path.join(device_config_dir, "buffers_config.j2"))
        self.assertDictEqual(
            output['CABLE_LENGTH'],
            {
                'AZURE': {
                    'Ethernet8': '0m',
                    'Ethernet0': '300m',
                    'Ethernet4': '300m',
                    'Ethernet-BP4': '5m',
                    'Ethernet-BP0': '5m',
                    'Ethernet-BP12': '5m',
                    'Ethernet-BP8': '5m',
                    'Ethernet12': '0m'
                }
            }
        )

    def test_buffers_chassis_packet_lc_template(self):
        build_root_dir = os.path.join(
            self.test_dir, "..", "..", ".."
        )
        # using T2 buffer configuration
        buffer_template = os.path.join(
            build_root_dir, "files", "build_templates", "buffers_config.j2"
        )
        minigraph = os.path.join(
            self.test_dir, "sample-chassis-packet-lc-graph.xml"
        )
        port_config_ini_asic1 = os.path.join(
            self.test_dir, "sample-chassis-packet-lc-port-config.ini"
        )
        device_config_dir = self.test_data_dir
        device_buffer_template = os.path.join(
            device_config_dir, "buffers.json.j2"
        )
        shutil.copy2(buffer_template, device_config_dir)
        # asic1 - mix of front end and back end ports
        argument = ["-m", minigraph, "-p", port_config_ini_asic1, "-n", "asic1", "-t", device_buffer_template]
        output = json.loads(self.run_script(argument, check_stderr=True))
        os.remove(os.path.join(device_config_dir, "buffers_config.j2"))
        self.assertDictEqual(
            output['CABLE_LENGTH'],
            {
                'AZURE': {
                    'Ethernet13': '300m',
                    'Ethernet14': '300m',
                    'Ethernet16': '300m',
                    'Ethernet17': '300m',
                    'Ethernet19': '300m',
                    'Ethernet20': '300m',
                    'Ethernet22': '300m',
                    'Ethernet23': '300m',
                    'Ethernet25': '300m',
                    'Ethernet26': '300m',
                    'Ethernet28': '300m',
                    'Ethernet29': '300m',
                    'Ethernet31': '300m',
                    'Ethernet32': '300m',
                    'Ethernet34': '300m',
                    'Ethernet35': '300m',
                    'Ethernet37': '300m',
                    'Ethernet38': '300m',
                    'Ethernet40': '300m',
                    'Ethernet41': '300m',
                    'Ethernet43': '300m',
                    'Ethernet44': '300m',
                    'Ethernet46': '300m',
                    'Ethernet47': '300m',
                    'Ethernet-BP2320': '1m',
                    'Ethernet-BP2452': '1m',
                    'Ethernet-BP2454': '1m',
                    'Ethernet-BP2456': '1m',
                    'Ethernet-BP2458': '1m',
                    'Ethernet-BP2460': '1m',
                    'Ethernet-BP2462': '1m',
                    'Ethernet-BP2464': '1m',
                    'Ethernet-BP2466': '1m',
                    'Ethernet-BP2468': '1m',
                    'Ethernet-BP2470': '1m',
                    'Ethernet-BP2472': '1m',
                    'Ethernet-BP2474': '1m',
                    'Ethernet-BP2476': '1m',
                    'Ethernet-BP2478': '1m',
                    'Ethernet-BP2480': '1m',
                    'Ethernet-BP2482': '1m',
                    'Ethernet-BP2484': '1m',
                    'Ethernet-BP2486': '1m',
                    'Ethernet-BP2488': '1m',
                    'Ethernet-BP2490': '1m',
                    'Ethernet-BP2492': '1m',
                    'Ethernet-BP2494': '1m',
                    'Ethernet-BP2496': '1m',
                    'Ethernet-BP2498': '1m',
                    'Ethernet-BP2500': '1m',
                    'Ethernet-BP2502': '1m',
                    'Ethernet-BP2504': '1m',
                    'Ethernet-BP2506': '1m',
                    'Ethernet-BP2508': '1m',
                    'Ethernet-BP2510': '1m',
                    'Ethernet-BP2512': '1m',
                    'Ethernet-BP2514': '1m',
                    'Ethernet-BP2516': '1m',
                    'Ethernet-BP2518': '1m'
                }
            }
        )

    def test_bgpd_frr_frontendasic(self):
        self.assertTrue(*self.run_frr_asic_case('bgpd/bgpd.conf.j2', 'bgpd_frr_frontend_asic.conf', "asic0", self.port_config[0]))

    def test_bgpd_frr_backendasic(self):
        self.assertTrue(*self.run_frr_asic_case('bgpd/bgpd.conf.j2', 'bgpd_frr_backend_asic.conf', "asic3", self.port_config[3]))

    def test_no_asic_in_graph(self):
        argument = ["-m", self.sample_graph, "-p", self.sample_no_asic_port_config, "-n", "asic4", "--var-json", "PORTCHANNEL"]
        output = json.loads(self.run_script(argument, check_stderr=False, validateYang=False))
        self.assertDictEqual(output, {})

    def tearDown(self):
        os.environ["CFGGEN_UNIT_TESTING"] = ""
