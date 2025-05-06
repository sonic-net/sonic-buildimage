import json
import os
import subprocess
import yaml
import tests.common_utils as utils

from unittest import TestCase
from sonic_py_common.general import getstatusoutput_noshell


class TestChassis(TestCase):

    def setUp(self):
        self.yang = utils.YangWrapper()
        self.test_dir = os.path.dirname(os.path.realpath(__file__))
        self.script_file = [utils.PYTHON_INTERPRETTER,
                            os.path.join(self.test_dir, '..', 'sonic-cfggen')]
        self.output_file = os.path.join(self.test_dir, 'output')
        self.macsec_profile = os.path.join(
            self.test_dir, 'macsec_profile.json')

    def run_script(self, argument, check_stderr=True, output_file=None, validateYang=True, ignore_warning=False):
        print('\n    Running sonic-cfggen ' + ' '.join(argument))
        # if validateYang:
        # self.assertTrue(self.yang.validate(argument))
        if check_stderr:
            output = subprocess.check_output(
                self.script_file + argument, stderr=subprocess.STDOUT)
        else:
            output = subprocess.check_output(self.script_file + argument)

        if utils.PY3x:
            output = output.decode()
        if output_file:
            with open(output_file, 'w') as f:
                f.write(output)

        if ignore_warning:
            output_without_warning = []
            for line in output.split('\n'):
                if line.startswith("Warning"):
                    continue
                output_without_warning.append(line)
            output = '\n'.join(output_without_warning)
            
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


class TestVoqChassisSingleAsic(TestChassis):
    def setUp(self):
        super().setUp()
        self.test_data_dir = os.path.join(
            self.test_dir,  'chassis_data/voq_chassis_data')
        self.sample_graph = os.path.join(
            self.test_data_dir, 'voq_chassis_lc_single_asic.xml')
        self.sample_port_config = os.path.join(
            self.test_data_dir, 'voq-sample-port-config-1.ini')
        os.environ['CFGGEN_UNIT_TESTING'] = '2'

    def test_dummy_run(self):
        argument = []
        output = self.run_script(argument)
        self.assertEqual(output, '')

    def test_print_data(self):
        argument = ['-m', self.sample_graph, '-p',
                    self.sample_port_config, '--print-data']
        output = self.run_script(argument)
        self.assertGreater(len(output.strip()), 0)

    def test_read_yaml(self):
        argument = ['-v', 'yml_item', '-y',
                    os.path.join(self.test_dir, 'test.yml')]
        output = yaml.safe_load(self.run_script(argument))
        self.assertListEqual(output, ['value1', 'value2'])

    def test_render_template(self):
        argument = ['-y', os.path.join(self.test_dir, 'test.yml'),
                    '-t', os.path.join(self.test_dir, 'test.j2')]
        output = self.run_script(argument)
        self.assertEqual(output.strip(), 'value1\nvalue2')

    def test_tacacs(self):
        argument = ['-m', self.sample_graph, '-p',
                    self.sample_port_config, '--var-json', 'TACPLUS_SERVER']
        output = json.loads(self.run_script(argument))
        self.assertDictEqual(
            output, {'123.46.98.21': {'priority': '1', 'tcp_port': '49'}})
        # TACPLUS_SERVER not present in the asic configuration.
        argument = ['-m', self.sample_graph, '--var-json', 'TACPLUS_SERVER']

    def test_ntp(self):
        argument = ['-m', self.sample_graph, '-p',
                    self.sample_port_config, '--var-json', 'NTP_SERVER']
        output = json.loads(self.run_script(argument))
        self.assertDictEqual(output, {'17.39.1.130': {'iburst': 'on'}, '17.39.1.129': {'iburst': 'on'}})
        # NTP data is present only in the host config
        argument = ['-m', self.sample_graph, '--var-json', 'NTP_SERVER']

    def test_mgmt_port(self):
        argument = ['-m', self.sample_graph, '-p',
                    self.sample_port_config, '--var-json', 'MGMT_PORT']
        output = json.loads(self.run_script(argument))
        self.assertDictEqual(
            output, {'eth0': {'alias': 'Management1/1', 'admin_status': 'up'}})

    def test_device_metadata(self):
        argument = ['-m', self.sample_graph, '-p',
                    self.sample_port_config, '--var-json', 'DEVICE_METADATA']
        output = json.loads(self.run_script(argument))
        print(output['localhost'])
        self.assertDictEqual(output['localhost'], {
            'bgp_asn': '65100',
            'region': 'test',
            'cloudtype': None,
            'docker_routing_config_mode': 'separated',
            'hostname': 'str-sonic-lc06',
            'hwsku': 'Sonic-lc-100g-sku',
            'type': 'SpineRouter',
            'synchronous_mode': 'enable',
            'yang_config_validation': 'disable',
            'chassis_hostname': 'str-sonic',
            'deployment_id': '3',
            'cluster': 'TestbedForstr-sonic',
            'asic_name': 'Asic0',
            'sub_role': 'FrontEnd',
            'switch_type': 'voq',
            'switch_id': 20,
            'max_cores': 64,
            'slice_type': 'AZNG_Production',
            'subtype': 'UpstreamLC'})

    def test_port(self):
        argument = ['-m', self.sample_graph, '-p',
                    self.sample_port_config, '-v', "PORT[\'Ethernet0\']"]
        output = self.run_script(argument)
        print(output)
        self.assertEqual(
            utils.to_dict(output.strip()),
            utils.to_dict("{'lanes': '6,7',\
                'alias': 'Ethernet6/1/1',\
                'index': '1',\
                'role': 'Ext',\
                'speed': '100000', \
                'asic_port_name': 'Eth0',\
                'core_id': '0', \
                'core_port_id': '1',\
                'num_voq': '8', \
                'fec': 'rs', \
                'description': 'ARISTA33T1:Ethernet1',\
                'mtu': '9100',\
                'tpid': '0x8100',\
                'pfc_asym': 'off',\
                'admin_status': 'up'\
            }")
        )

    def test_voq_inband_intf(self):
        argument = ['-m', self.sample_graph, '-p',
                    self.sample_port_config, '--var-json', 'VOQ_INBAND_INTERFACE']
        output = self.run_script(argument)
        print(output)
        self.assertEqual(
            utils.to_dict(output.strip()),
            utils.to_dict("{'Ethernet-IB0': {'inband_type': 'Port'},\
                'Ethernet-IB0|10.241.88.4/32': {},\
                'Ethernet-IB0|2a01:111:e210:5e:0:a:f158:400/128': {}}"))

    def test_system_port(self):
        argument = ['-m', self.sample_graph, '-p',
                    self.sample_port_config, '--var-json', 'SYSTEM_PORT']
        expected_output_file = os.path.join(
            self.test_data_dir, 'system_ports.json')
        self.run_script(argument, output_file=self.output_file)
        self.assertFalse(self.run_diff(expected_output_file, self.output_file))
        if os.path.exists(self.output_file):
            os.remove(self.output_file)

    def test_device_neighbor(self):
        argument = ['-m', self.sample_graph, '-p',
                    self.sample_port_config, '--var-json', 'DEVICE_NEIGHBOR']
        output = self.run_script(argument)
        print(output)
        self.assertEqual(
            utils.to_dict(output.strip()),
            utils.to_dict("{'Ethernet0': {'name': 'ARISTA33T1', 'port':'Ethernet1'},\
                            'Ethernet4': {'name': 'ARISTA33T1', 'port':'Ethernet2'},\
                            'Ethernet8': {'name': 'ARISTA35T1', 'port':'Ethernet1'},\
                            'Ethernet12': {'name': 'ARISTA35T1', 'port':'Ethernet2'},\
                            'Ethernet16': {'name': 'ARISTA37T1', 'port':'Ethernet1'},\
                            'Ethernet20': {'name': 'ARISTA37T1', 'port':'Ethernet2'},\
                            'Ethernet24': {'name': 'ARISTA39T1', 'port':'Ethernet1'},\
                            'Ethernet28': {'name': 'ARISTA39T1', 'port':'Ethernet2'},\
                            'Ethernet32': {'name': 'ARISTA41T1', 'port':'Ethernet1'},\
                            'Ethernet36': {'name': 'ARISTA41T1', 'port':'Ethernet2'},\
                            'Ethernet40': {'name': 'ARISTA43T1', 'port':'Ethernet1'},\
                            'Ethernet44': {'name': 'ARISTA43T1', 'port':'Ethernet2'},\
                            'Ethernet48': {'name': 'ARISTA45T1', 'port':'Ethernet1'},\
                            'Ethernet52': {'name': 'ARISTA47T1', 'port':'Ethernet1'},\
                            'Ethernet56': {'name': 'ARISTA47T1', 'port':'Ethernet2'},\
                            'Ethernet60': {'name': 'ARISTA49T1', 'port':'Ethernet1'},\
                            'Ethernet64': {'name': 'ARISTA50T1', 'port':'Ethernet1'},\
                            'Ethernet68': {'name': 'ARISTA50T1', 'port':'Ethernet2'},\
                            'Ethernet72': {'name': 'ARISTA51T1', 'port':'Ethernet1'},\
                            'Ethernet76': {'name': 'ARISTA52T1', 'port':'Ethernet1'},\
                            'Ethernet80': {'name': 'ARISTA53T1', 'port':'Ethernet1'},\
                            'Ethernet84': {'name': 'ARISTA54T1', 'port':'Ethernet1'},\
                            'Ethernet88': {'name': 'ARISTA55T1', 'port':'Ethernet1'},\
                            'Ethernet92': {'name': 'ARISTA56T1', 'port':'Ethernet1'},\
                            'Ethernet96': {'name': 'ARISTA57T1', 'port':'Ethernet1'},\
                            'Ethernet100': {'name': 'ARISTA58T1', 'port':'Ethernet1'},\
                            'Ethernet104': {'name': 'ARISTA59T1', 'port':'Ethernet1'},\
                            'Ethernet108': {'name': 'ARISTA60T1', 'port':'Ethernet1'},\
                            'Ethernet112': {'name': 'ARISTA61T1', 'port':'Ethernet1'},\
                            'Ethernet116': {'name': 'ARISTA62T1', 'port':'Ethernet1'},\
                            'Ethernet120': {'name': 'ARISTA63T1', 'port':'Ethernet1'},\
                            'Ethernet124': {'name': 'ARISTA64T1', 'port': 'Ethernet1'}}"))

    def test_device_neighbor_metadata(self):
        argument = ['-m', self.sample_graph, '-p',
                    self.sample_port_config, '-v', "DEVICE_NEIGHBOR_METADATA[\'ARISTA33T1\']"]
        output = self.run_script(argument)
        print(output)
        self.assertEqual(
            utils.to_dict(output.strip()),
            utils.to_dict("{'hwsku': 'Sonic-T1-sku',\
                'cluster': 'TestbedForstr-sonic',\
                'deployment_id': '3',\
                'lo_addr': '0.0.0.0/0',\
                'lo_addr_v6': '::/0',\
                'mgmt_addr': '172.16.191.58/17',\
                'mgmt_addr_v6': '::/0',\
                'type': 'LeafRouter'}"))

    def test_bgp_neighbors(self):
        argument = ['-m', self.sample_graph, '-p',
                    self.sample_port_config, '-v', 'BGP_NEIGHBOR']
        output = self.run_script(argument)
        print(output)
        self.assertEqual(
            utils.to_dict(output.strip()),
            utils.to_dict("{'10.0.0.129': {'name': 'ARISTA33T1', 'local_addr': '10.0.0.128', 'rrclient': 0, 'holdtime': '10', 'keepalive': '3', 'nhopself': 0, 'asn': '65338'},\
                'fc00::102': {'name': 'ARISTA33T1', 'local_addr': 'fc00::101', 'rrclient': 0, 'holdtime': '10', 'keepalive': '3', 'nhopself': 0, 'asn': '65338'},\
                '10.0.0.133': {'name': 'ARISTA35T1', 'local_addr': '10.0.0.132', 'rrclient': 0, 'holdtime': '10', 'keepalive': '3', 'nhopself': 0, 'asn': '65339'},\
                'fc00::10a': {'name': 'ARISTA35T1', 'local_addr': 'fc00::109', 'rrclient': 0, 'holdtime': '10', 'keepalive': '3', 'nhopself': 0, 'asn': '65339'},\
                '10.0.0.137': {'name': 'ARISTA37T1', 'local_addr': '10.0.0.136', 'rrclient': 0, 'holdtime': '10', 'keepalive': '3', 'nhopself': 0, 'asn': '65340'},\
                'fc00::112': {'name': 'ARISTA37T1', 'local_addr': 'fc00::111', 'rrclient': 0, 'holdtime': '10', 'keepalive': '3', 'nhopself': 0, 'asn': '65340'},\
                '10.0.0.141': {'name': 'ARISTA39T1', 'local_addr': '10.0.0.140', 'rrclient': 0, 'holdtime': '10', 'keepalive': '3', 'nhopself': 0, 'asn': '65341'},\
                'fc00::11a': {'name': 'ARISTA39T1', 'local_addr': 'fc00::119', 'rrclient': 0, 'holdtime': '10', 'keepalive': '3', 'nhopself': 0, 'asn': '65341'},\
                '10.0.0.145': {'name': 'ARISTA41T1', 'local_addr': '10.0.0.144', 'rrclient': 0, 'holdtime': '10', 'keepalive': '3', 'nhopself': 0, 'asn': '65342'},\
                'fc00::122': {'name': 'ARISTA41T1', 'local_addr': 'fc00::121', 'rrclient': 0, 'holdtime': '10', 'keepalive': '3', 'nhopself': 0, 'asn': '65342'},\
                '10.0.0.149': {'name': 'ARISTA43T1', 'local_addr': '10.0.0.148', 'rrclient': 0, 'holdtime': '10', 'keepalive': '3', 'nhopself': 0, 'asn': '65343'},\
                'fc00::12a': {'name': 'ARISTA43T1', 'local_addr': 'fc00::129', 'rrclient': 0, 'holdtime': '10', 'keepalive': '3', 'nhopself': 0, 'asn': '65343'},\
                '10.0.0.153': {'name': 'ARISTA47T1', 'local_addr': '10.0.0.152', 'rrclient': 0, 'holdtime': '10', 'keepalive': '3', 'nhopself': 0, 'asn': '65345'},\
                'fc00::132': {'name': 'ARISTA47T1', 'local_addr': 'fc00::131', 'rrclient': 0, 'holdtime': '10', 'keepalive': '3', 'nhopself': 0, 'asn': '65345'},\
                '10.0.0.157': {'name': 'ARISTA50T1', 'local_addr': '10.0.0.156', 'rrclient': 0, 'holdtime': '10', 'keepalive': '3', 'nhopself': 0, 'asn': '65347'},\
                'fc00::13a': {'name': 'ARISTA50T1', 'local_addr': 'fc00::139', 'rrclient': 0, 'holdtime': '10', 'keepalive': '3', 'nhopself': 0, 'asn': '65347'}}"))

    def test_voq_bgp_neighbors(self):
        argument = ['-m', self.sample_graph, '-p',
                    self.sample_port_config, '-v', 'BGP_VOQ_CHASSIS_NEIGHBOR']
        output = self.run_script(argument)
        print(output)
        self.assertEqual(
            utils.to_dict(output.strip()),
            utils.to_dict(
                "{'10.241.88.5': {'name': 'str-sonic-lc07-ASIC00', 'local_addr': '10.241.88.4', 'rrclient': 0, 'holdtime': '0', 'keepalive': '0', 'nhopself': 0, 'admin_status': 'up', 'asn': '65100'}, "
                "'2a01:111:e210:5e:0:a:f158:500': {'name': 'str-sonic-lc07-ASIC00', 'local_addr': '2a01:111:e210:5e:0:a:f158:400', 'rrclient': 0, 'holdtime': '0', 'keepalive': '0', 'nhopself': 0, 'admin_status': 'up', 'asn': '65100'}, "
                "'10.241.88.6': {'name': 'str-sonic-lc08-ASIC00', 'local_addr': '10.241.88.4', 'rrclient': 0, 'holdtime': '0', 'keepalive': '0', 'nhopself': 0, 'admin_status': 'up', 'asn': '65100'}, "
                "'2a01:111:e210:5e:0:a:f158:600': {'name': 'str-sonic-lc08-ASIC00', 'local_addr': '2a01:111:e210:5e:0:a:f158:400', 'rrclient': 0, 'holdtime': '0', 'keepalive': '0', 'nhopself': 0, 'admin_status': 'up', 'asn': '65100'}, "
                "'10.241.88.7': {'name': 'str-sonic-lc09-ASIC00', 'local_addr': '10.241.88.4', 'rrclient': 0, 'holdtime': '0', 'keepalive': '0', 'nhopself': 0, 'admin_status': 'up', 'asn': '65100'}, "
                "'2a01:111:e210:5e:0:a:f158:700': {'name': 'str-sonic-lc09-ASIC00', 'local_addr': '2a01:111:e210:5e:0:a:f158:400', 'rrclient': 0, 'holdtime': '0', 'keepalive': '0', 'nhopself': 0, 'admin_status': 'up', 'asn': '65100'}, "
                "'10.241.88.8': {'name': 'str-sonic-lc10-ASIC00', 'local_addr': '10.241.88.4', 'rrclient': 0, 'holdtime': '0', 'keepalive': '0', 'nhopself': 0, 'admin_status': 'up', 'asn': '65100'}, "
                "'2a01:111:e210:5e:0:a:f158:800': {'name': 'str-sonic-lc10-ASIC00', 'local_addr': '2a01:111:e210:5e:0:a:f158:400', 'rrclient': 0, 'holdtime': '0', 'keepalive': '0', 'nhopself': 0, 'admin_status': 'up', 'asn': '65100'}, "
                "'10.241.88.0': {'name': 'str-sonic-lc03-ASIC00', 'local_addr': '10.241.88.4', 'rrclient': 0, 'holdtime': '0', 'keepalive': '0', 'nhopself': 0, 'admin_status': 'up', 'asn': '65100'}, "
                "'2a01:111:e210:5e:0:a:f158:0': {'name': 'str-sonic-lc03-ASIC00', 'local_addr': '2a01:111:e210:5e:0:a:f158:400', 'rrclient': 0, 'holdtime': '0', 'keepalive': '0', 'nhopself': 0, 'admin_status': 'up', 'asn': '65100'}, "
                "'10.241.88.1': {'name': 'str-sonic-lc03-ASIC01', 'local_addr': '10.241.88.4', 'rrclient': 0, 'holdtime': '0', 'keepalive': '0', 'nhopself': 0, 'admin_status': 'up', 'asn': '65100'}, "
                "'2a01:111:e210:5e:0:a:f158:100': {'name': 'str-sonic-lc03-ASIC01', 'local_addr': '2a01:111:e210:5e:0:a:f158:400', 'rrclient': 0, 'holdtime': '0', 'keepalive': '0', 'nhopself': 0, 'admin_status': 'up', 'asn': '65100'}, "
                "'10.241.88.2': {'name': 'str-sonic-lc04-ASIC00', 'local_addr': '10.241.88.4', 'rrclient': 0, 'holdtime': '0', 'keepalive': '0', 'nhopself': 0, 'admin_status': 'up', 'asn': '65100'}, "
                "'2a01:111:e210:5e:0:a:f158:200': {'name': 'str-sonic-lc04-ASIC00', 'local_addr': '2a01:111:e210:5e:0:a:f158:400', 'rrclient': 0, 'holdtime': '0', 'keepalive': '0', 'nhopself': 0, 'admin_status': 'up', 'asn': '65100'}, "
                "'10.241.88.3': {'name': 'str-sonic-lc05-ASIC00', 'local_addr': '10.241.88.4', 'rrclient': 0, 'holdtime': '0', 'keepalive': '0', 'nhopself': 0, 'admin_status': 'up', 'asn': '65100'}, "
                "'2a01:111:e210:5e:0:a:f158:300': {'name': 'str-sonic-lc05-ASIC00', 'local_addr': '2a01:111:e210:5e:0:a:f158:400', 'rrclient': 0, 'holdtime': '0', 'keepalive': '0', 'nhopself': 0, 'admin_status': 'up', 'asn': '65100'}}")
        )

    def test_port_channel(self):
        argument = ['-m', self.sample_graph, '-p',
                    self.sample_port_config, '-v', "PORTCHANNEL[\'PortChannel1058\']"]
        output = self.run_script(argument)
        print(output)
        self.assertEqual(
            utils.to_dict(output.strip()),
            utils.to_dict("{'min_links': '2', 'lacp_key': 'auto', 'mtu': '9100', 'tpid': '0x8100', 'admin_status': 'up'}"))

    def test_port_channel_member(self):
        argument = ['-m', self.sample_graph, '-p',
                    self.sample_port_config, '-v', "PORTCHANNEL_MEMBER.keys()|list"]
        output = self.run_script(argument)
        print(output)
        self.assertEqual(
            utils.liststr_to_dict(output.strip()),
            utils.liststr_to_dict("[('PortChannel1049', 'Ethernet0'),\
                ('PortChannel1049', 'Ethernet4'),\
                ('PortChannel1050', 'Ethernet8'),\
                ('PortChannel1050', 'Ethernet12'),\
                ('PortChannel1051', 'Ethernet16'),\
                ('PortChannel1051', 'Ethernet20'),\
                ('PortChannel1052', 'Ethernet24'),\
                ('PortChannel1052', 'Ethernet28'),\
                ('PortChannel1053', 'Ethernet36'),\
                ('PortChannel1053', 'Ethernet32'),\
                ('PortChannel1054', 'Ethernet40'),\
                ('PortChannel1054', 'Ethernet44'),\
                ('PortChannel1056', 'Ethernet52'),\
                ('PortChannel1056', 'Ethernet56'),\
                ('PortChannel1058', 'Ethernet64'),\
                ('PortChannel1058', 'Ethernet68')]")
        )

    def test_port_channel_interface(self):
        argument = ['-m', self.sample_graph, '-p',
                    self.sample_port_config, '-v', "PORTCHANNEL_INTERFACE.keys()|list"]
        output = self.run_script(argument)
        print(output)
        self.assertEqual(
            utils.liststr_to_dict(output.strip()),
            utils.liststr_to_dict("\
                [('PortChannel1049', '10.0.0.128/31'),\
                'PortChannel1049',\
                ('PortChannel1049', 'fc00::101/126'),\
                ('PortChannel1050', '10.0.0.132/31'),\
                'PortChannel1050',\
                ('PortChannel1050', 'fc00::109/126'),\
                ('PortChannel1051', '10.0.0.136/31'),\
                'PortChannel1051',\
                ('PortChannel1051', 'fc00::111/126'),\
                ('PortChannel1052', '10.0.0.140/31'),\
                'PortChannel1052',\
                ('PortChannel1052', 'fc00::119/126'),\
                ('PortChannel1053', '10.0.0.144/31'),\
                'PortChannel1053',\
                ('PortChannel1053', 'fc00::121/126'),\
                ('PortChannel1054', '10.0.0.148/31'),\
                'PortChannel1054',\
                ('PortChannel1054', 'fc00::129/126'),\
                ('PortChannel1056', '10.0.0.152/31'),\
                'PortChannel1056',\
                ('PortChannel1056', 'fc00::131/126'),\
                ('PortChannel1058', '10.0.0.156/31'),\
                'PortChannel1058',\
                ('PortChannel1058', 'fc00::139/126')\
                ]")
        )

    def tearDown(self):
        os.environ['CFGGEN_UNIT_TESTING'] = ''
        if os.path.exists(self.output_file):
            os.remove(self.output_file)


class TestVoqChassisMultiAsic(TestChassis):

    def setUp(self):
        super().setUp()
        self.test_data_dir = os.path.join(
            self.test_dir,  'chassis_data/voq_chassis_data')
        self.sample_graph = os.path.join(
            self.test_data_dir, 'voq_chassis_lc_multi_asic.xml')
        self.sample_port_config = os.path.join(
            self.test_data_dir, 'voq-sample-port-config-2.ini')
        os.environ['CFGGEN_UNIT_TESTING'] = '2'
        os.environ["CFGGEN_UNIT_TESTING_TOPOLOGY"] = "multi_asic"

    def test_dummy_run(self):
        argument = []
        output = self.run_script(argument)
        self.assertEqual(output, '')

    def test_print_data(self):
        argument = ['-m', self.sample_graph, '-p',
                    self.sample_port_config, '--print-data']
        output = self.run_script(argument)
        self.assertGreater(len(output.strip()), 0)

    def test_read_yaml(self):
        argument = ['-v', 'yml_item', '-y',
                    os.path.join(self.test_dir, 'test.yml')]
        output = yaml.safe_load(self.run_script(argument))
        self.assertListEqual(output, ['value1', 'value2'])

    def test_render_template(self):
        argument = ['-y', os.path.join(self.test_dir, 'test.yml'),
                    '-t', os.path.join(self.test_dir, 'test.j2')]
        output = self.run_script(argument)
        self.assertEqual(output.strip(), 'value1\nvalue2')

    def test_tacacs(self):
        argument = ['-m', self.sample_graph, '-p',
                    self.sample_port_config, '--var-json', 'TACPLUS_SERVER']
        output = json.loads(self.run_script(argument))
        self.assertDictEqual(
            output, {'123.46.98.21': {'priority': '1', 'tcp_port': '49'}})
        # TACPLUS_SERVER not present in the asic configuration.
        argument = ['-m', self.sample_graph, '--var-json', 'TACPLUS_SERVER']

    def test_ntp(self):
        argument = ['-m', self.sample_graph, '-p',
                    self.sample_port_config, '--var-json', 'NTP_SERVER']
        output = json.loads(self.run_script(argument))
        self.assertDictEqual(output, {'17.39.1.130': {'iburst': 'on'}, '17.39.1.129': {'iburst': 'on'}})
        # NTP data is present only in the host config
        argument = ['-m', self.sample_graph, '--var-json', 'NTP_SERVER']

    def test_mgmt_port(self):
        argument = ['-m', self.sample_graph, '-p',
                    self.sample_port_config, '--var-json', 'MGMT_PORT']
        output = json.loads(self.run_script(argument))
        self.assertDictEqual(
            output, {'eth0': {'alias': 'Management1/1', 'admin_status': 'up'}})

    def test_device_metadata(self):
        argument = ['-m', self.sample_graph, '-p',
                    self.sample_port_config, '--var-json', 'DEVICE_METADATA']
        output = json.loads(self.run_script(argument))
        print(output['localhost'])
        self.assertDictEqual(output['localhost'], {
            'region': 'test',
            'cloudtype': 'Public',
            'docker_routing_config_mode': 'separated',
            'hostname': 'str-sonic-lc03',
            'hwsku': 'Sonic-400g-lc-sku',
            'type': 'SpineRouter',
            'synchronous_mode': 'enable',
            'yang_config_validation': 'disable',
            'chassis_hostname': 'str-sonic',
            'deployment_id': '3',
            'cluster': 'TestbedForstr-sonic',
            'subtype': 'DownstreamLC',
            'switch_type': 'voq',
            'max_cores': 64,
            })

    def test_device_metadata_for_namespace(self):
        argument = [
            '-m', self.sample_graph,
            '-p', self.sample_port_config,
            '-n', 'asic0',
            '--var-json', 'DEVICE_METADATA'
        ]
        output = json.loads(self.run_script(argument))
        print(output['localhost'])
        self.assertDictEqual(output['localhost'], {
            'bgp_asn': '65100',
            'region': 'test',
            'cloudtype': None,
            'docker_routing_config_mode': 'separated',
            'hostname': 'str-sonic-lc03',
            'hwsku': 'Sonic-400g-lc-sku',
            'type': 'SpineRouter',
            'synchronous_mode': 'enable',
            'yang_config_validation': 'disable',
            'chassis_hostname': 'str-sonic',
            'deployment_id': '3', 'cluster':
            'TestbedForstr-sonic',
            'sub_role': 'FrontEnd',
            'asic_name': 'asic0',
            'switch_type': 'voq',
            'switch_id': 8,
            'max_cores': 64,
            'subtype': 'DownstreamLC'})

    def test_system_port(self):
        argument = ['-m', self.sample_graph,
                    '-p', self.sample_port_config,
                    '-n', 'asic0',
                    '--var-json', 'SYSTEM_PORT']
        expected_output_file = os.path.join(
            self.test_data_dir, 'system_ports.json')
        self.run_script(argument, output_file=self.output_file)
        self.assertFalse(self.run_diff(expected_output_file, self.output_file))
        if os.path.exists(self.output_file):
            os.remove(self.output_file)

    def test_port(self):
        argument = [
            '-j', self.macsec_profile,
            '-m', self.sample_graph,
            '-p', self.sample_port_config,
            '-n', 'asic0',
            '-v', "PORT[\'Ethernet8\']"
        ]
        output = self.run_script(argument)
        print(output)
        self.assertEqual(
            utils.to_dict(output.strip()),
            utils.to_dict("{"
                          "'lanes': '80,81,82,83', "
                          "'alias': 'Ethernet3/2/1', "
                          "'index': '2', "
                          "'role': 'Ext', "
                          "'speed': '100000', "
                          "'asic_port_name': 'Eth8', "
                          "'core_id': '1', "
                          "'core_port_id': '2', "
                          "'num_voq': '8', "
                          "'fec': 'rs', "
                          "'macsec': 'macsec-profile-two', "
                          "'tx_power': '-10', "
                          "'laser_freq': 196025, "
                          "'description': 'ARISTA91T3:Ethernet2', "
                          "'mtu': '9100', "
                          "'tpid': '0x8100', "
                          "'pfc_asym': 'off', "
                          "'admin_status': 'up'"
                          "}"
                          )
        )

    def test_voq_inband_intf(self):
        argument = [
            '-j', self.macsec_profile,
            '-m', self.sample_graph,
            '-p', self.sample_port_config,
            '-n', 'asic1',
            '--var-json', 'VOQ_INBAND_INTERFACE'
        ]
        output = self.run_script(argument)
        print(output)
        self.assertEqual(
            utils.to_dict(output.strip()),
            utils.to_dict("{'Ethernet-IB1': {'inband_type': 'Port'},\
                'Ethernet-IB1|10.241.88.1/32': {},\
                'Ethernet-IB1|2a01:111:e210:5e:0:a:f158:100/128': {}}"))

    def test_voq_bgp_neighbors(self):
        argument = [
            '-m', self.sample_graph,
            '-p', self.sample_port_config,
            '-n', 'asic0',
            '--var-json', 'BGP_VOQ_CHASSIS_NEIGHBOR'
        ]
        output = json.loads(self.run_script(argument))
        print(output)
        self.assertDictEqual(output,
            {
                "10.241.88.1": {
                    "name": "str-sonic-lc03-ASIC01",
                    "local_addr": "10.241.88.0",
                    "rrclient": 0,
                    "holdtime": "0",
                    "keepalive": "0",
                    "nhopself": 0,
                    "admin_status": "up",
                    "asn": "65100"
                },
                "2a01:111:e210:5e:0:a:f158:100": {
                    "name": "str-sonic-lc03-ASIC01",
                    "local_addr": "2a01:111:e210:5e:0:a:f158:0",
                    "rrclient": 0,
                    "holdtime": "0",
                    "keepalive": "0",
                    "nhopself": 0,
                    "admin_status": "up",
                    "asn": "65100"
                },
                "10.241.88.2": {
                    "name": "str-sonic-lc04-ASIC00",
                    "local_addr": "10.241.88.0",
                    "rrclient": 0,
                    "holdtime": "0",
                    "keepalive": "0",
                    "nhopself": 0,
                    "admin_status": "up",
                    "asn": "65100"
                },
                "2a01:111:e210:5e:0:a:f158:200": {
                    "name": "str-sonic-lc04-ASIC00",
                    "local_addr": "2a01:111:e210:5e:0:a:f158:0",
                    "rrclient": 0,
                    "holdtime": "0",
                    "keepalive": "0",
                    "nhopself": 0,
                    "admin_status": "up",
                    "asn": "65100"
                },
                "10.241.88.3": {
                    "name": "str-sonic-lc05-ASIC00",
                    "local_addr": "10.241.88.0",
                    "rrclient": 0,
                    "holdtime": "0",
                    "keepalive": "0",
                    "nhopself": 0,
                    "admin_status": "up",
                    "asn": "65100"
                },
                "2a01:111:e210:5e:0:a:f158:300": {
                    "name": "str-sonic-lc05-ASIC00",
                    "local_addr": "2a01:111:e210:5e:0:a:f158:0",
                    "rrclient": 0,
                    "holdtime": "0",
                    "keepalive": "0",
                    "nhopself": 0,
                    "admin_status": "up",
                    "asn": "65100"
                },
                "10.241.88.4": {
                    "name": "str-sonic-lc06-ASIC00",
                    "local_addr": "10.241.88.0",
                    "rrclient": 0,
                    "holdtime": "0",
                    "keepalive": "0",
                    "nhopself": 0,
                    "admin_status": "up",
                    "asn": "65100"
                },
                "2a01:111:e210:5e:0:a:f158:400": {
                    "name": "str-sonic-lc06-ASIC00",
                    "local_addr": "2a01:111:e210:5e:0:a:f158:0",
                    "rrclient": 0,
                    "holdtime": "0",
                    "keepalive": "0",
                    "nhopself": 0,
                    "admin_status": "up",
                    "asn": "65100"
                },
                "10.241.88.5": {
                    "name": "str-sonic-lc07-ASIC00",
                    "local_addr": "10.241.88.0",
                    "rrclient": 0,
                    "holdtime": "0",
                    "keepalive": "0",
                    "nhopself": 0,
                    "admin_status": "up",
                    "asn": "65100"
                },
                "2a01:111:e210:5e:0:a:f158:500": {
                    "name": "str-sonic-lc07-ASIC00",
                    "local_addr": "2a01:111:e210:5e:0:a:f158:0",
                    "rrclient": 0,
                    "holdtime": "0",
                    "keepalive": "0",
                    "nhopself": 0,
                    "admin_status": "up",
                    "asn": "65100"
                },
                "10.241.88.6": {
                    "name": "str-sonic-lc08-ASIC00",
                    "local_addr": "10.241.88.0",
                    "rrclient": 0,
                    "holdtime": "0",
                    "keepalive": "0",
                    "nhopself": 0,
                    "admin_status": "up",
                    "asn": "65100"
                },
                "2a01:111:e210:5e:0:a:f158:600": {
                    "name": "str-sonic-lc08-ASIC00",
                    "local_addr": "2a01:111:e210:5e:0:a:f158:0",
                    "rrclient": 0,
                    "holdtime": "0",
                    "keepalive": "0",
                    "nhopself": 0,
                    "admin_status": "up",
                    "asn": "65100"
                },
                "10.241.88.7": {
                    "name": "str-sonic-lc09-ASIC00",
                    "local_addr": "10.241.88.0",
                    "rrclient": 0,
                    "holdtime": "0",
                    "keepalive": "0",
                    "nhopself": 0,
                    "admin_status": "up",
                    "asn": "65100"
                },
                "2a01:111:e210:5e:0:a:f158:700": {
                    "name": "str-sonic-lc09-ASIC00",
                    "local_addr": "2a01:111:e210:5e:0:a:f158:0",
                    "rrclient": 0,
                    "holdtime": "0",
                    "keepalive": "0",
                    "nhopself": 0,
                    "admin_status": "up",
                    "asn": "65100"
                },
                "10.241.88.8": {
                    "name": "str-sonic-lc10-ASIC00",
                    "local_addr": "10.241.88.0",
                    "rrclient": 0,
                    "holdtime": "0",
                    "keepalive": "0",
                    "nhopself": 0,
                    "admin_status": "up",
                    "asn": "65100"
                },
                "2a01:111:e210:5e:0:a:f158:800": {
                    "name": "str-sonic-lc10-ASIC00",
                    "local_addr": "2a01:111:e210:5e:0:a:f158:0",
                    "rrclient": 0,
                    "holdtime": "0",
                    "keepalive": "0",
                    "nhopself": 0,
                    "admin_status": "up",
                    "asn": "65100"
                }
            })


    def test_device_neighbor(self):
        argument = [
            '-m', self.sample_graph, 
            '-p', self.sample_port_config,
            '-n', 'asic0',
            '-v', "DEVICE_NEIGHBOR[\'Ethernet0\']"]
        
        output = self.run_script(argument)
        print(output)
        self.assertEqual(
            utils.to_dict(output.strip()),
            utils.to_dict("{'name': 'ARISTA91T3', 'port': 'Ethernet1'}"))

    def test_device_neighbor_metadata(self):
        argument = [
            '-m', self.sample_graph, 
            '-p', self.sample_port_config,
            '-n', 'asic0',
            '-v', "DEVICE_NEIGHBOR_METADATA['ARISTA91T3']"
        ]
        output = self.run_script(argument)
        print(output)
        self.assertEqual(
            utils.to_dict(output.strip()),
            utils.to_dict(
                "{'hwsku': 'Sonic-T3-sku', 'cluster': 'TestbedForstr-sonic', "
                "'deployment_id': '3', 'lo_addr': '0.0.0.0/0', "
                "'lo_addr_v6': '::/0', 'mgmt_addr': '172.16.191.10/17', "
                "'mgmt_addr_v6': '::/0', 'type': 'RegionalHub'}"
            )
        )

    def test_port_channel(self):
        argument = [
            '-m', self.sample_graph, 
            '-p', self.sample_port_config,
            '-n', 'asic0',
            '-v', "PORTCHANNEL[\'PortChannel102\']"
        ]
        output = self.run_script(argument)
        print(output)
        self.assertEqual(
            utils.to_dict(output.strip()),
            utils.to_dict("{'min_links': '2', 'lacp_key': 'auto', 'mtu': '9100', 'tpid': '0x8100', 'admin_status': 'up'}"))

    def test_port_channel_member(self):
        argument = [
            '-m', self.sample_graph, 
            '-p', self.sample_port_config,
            '-n', 'asic0',
            '-v', "PORTCHANNEL_MEMBER.keys()|list"
        ]
        output = self.run_script(argument)
        print(output)
        self.assertEqual(
            utils.liststr_to_dict(output.strip()),
            utils.liststr_to_dict("[('PortChannel102', 'Ethernet0'),\
                ('PortChannel102', 'Ethernet8'),\
                ('PortChannel104', 'Ethernet16'),\
                ('PortChannel104', 'Ethernet24'),\
                ('PortChannel106', 'Ethernet32'),\
                ('PortChannel106', 'Ethernet40'),\
                ('PortChannel108', 'Ethernet48'),\
                ('PortChannel108', 'Ethernet56'),\
                ('PortChannel1010', 'Ethernet72'),\
                ('PortChannel1010', 'Ethernet64'),\
                ('PortChannel1012', 'Ethernet80'),\
                ('PortChannel1012', 'Ethernet88'),\
                ('PortChannel1016', 'Ethernet104'),\
                ('PortChannel1016', 'Ethernet112'),\
                ('PortChannel1020', 'Ethernet128'),\
                ('PortChannel1020', 'Ethernet136')]")
        )

    def test_port_channel_interface(self):
        argument = [
            '-m', self.sample_graph, 
            '-p', self.sample_port_config,
            '-n', 'asic0',
            '-v', "PORTCHANNEL_INTERFACE.keys()|list"
            ]
        output = self.run_script(argument)
        print(output)
        self.assertEqual(
            utils.liststr_to_dict(output.strip()),
            utils.liststr_to_dict("[('PortChannel102', '10.0.0.0/31'),\
                'PortChannel102',\
                ('PortChannel102', 'fc00::1/126'),\
                ('PortChannel104', '10.0.0.4/31'),\
                'PortChannel104',\
                ('PortChannel104', 'fc00::9/126'),\
                ('PortChannel106', '10.0.0.8/31'),\
                'PortChannel106',\
                ('PortChannel106','fc00::11/126'),\
                ('PortChannel108', '10.0.0.12/31'),\
                'PortChannel108',\
                ('PortChannel108', 'fc00::19/126'),\
                ('PortChannel1010', '10.0.0.16/31'),\
                'PortChannel1010',\
                ('PortChannel1010', 'fc00::21/126'),\
                ('PortChannel1012', '10.0.0.20/31'),\
                'PortChannel1012',\
                ('PortChannel1012', 'fc00::29/126'),\
                ('PortChannel1016', '10.0.0.28/31'),\
                'PortChannel1016', \
                ('PortChannel1016', 'fc00::39/126'),\
                ('PortChannel1020', '10.0.0.24/31'),\
                'PortChannel1020',\
                ('PortChannel1020', 'fc00::31/126')]")
        )
    def tearDown(self):
        os.environ['CFGGEN_UNIT_TESTING'] = ''
        os.environ["CFGGEN_UNIT_TESTING_TOPOLOGY"] = ""
        if os.path.exists(self.output_file):
            os.remove(self.output_file)


class TestVoqChassisSup(TestChassis):

    def setUp(self):
        super().setUp()
        self.test_data_dir = os.path.join(
            self.test_dir,  'chassis_data/voq_chassis_data')
        self.sample_graph = os.path.join(
            self.test_data_dir, 'voq_chassis_sup.xml')
        self.sample_port_config = ""
        os.environ['CFGGEN_UNIT_TESTING'] = '2'
        os.environ["CFGGEN_UNIT_TESTING_TOPOLOGY"] = "multi_asic"

    def test_dummy_run(self):
        argument = []
        output = self.run_script(argument)
        self.assertEqual(output, '')

    def test_print_data(self):
        argument = [
            '-m', self.sample_graph, 
            '-p', self.sample_port_config,
            '--print-data'
        ]
        output = self.run_script(argument)
        self.assertGreater(len(output.strip()), 0)

    def test_read_yaml(self):
        argument = ['-v', 'yml_item', '-y',
                    os.path.join(self.test_dir, 'test.yml')]
        output = yaml.safe_load(self.run_script(argument))
        self.assertListEqual(output, ['value1', 'value2'])

    def test_render_template(self):
        argument = ['-y', os.path.join(self.test_dir, 'test.yml'),
                    '-t', os.path.join(self.test_dir, 'test.j2')]
        output = self.run_script(argument)
        self.assertEqual(output.strip(), 'value1\nvalue2')

    def test_tacacs(self):
        argument = [
            '-m', self.sample_graph,
            '-p', self.sample_port_config,
            '--var-json', 'TACPLUS_SERVER'
        ]
        output = json.loads(self.run_script(argument))
        self.assertDictEqual(
            output, {'123.46.98.21': {'priority': '1', 'tcp_port': '49'}})

    def test_ntp(self):
        argument = [
            '-m', self.sample_graph,
            '-p', self.sample_port_config,
            '--var-json', 'NTP_SERVER'
        ]
        output = json.loads(self.run_script(argument))
        self.assertDictEqual(output, {'17.39.1.130': {'iburst': 'on'}, '17.39.1.129': {'iburst': 'on'}})


    def test_mgmt_port(self):
        argument = [
            '-m', self.sample_graph,
            '-p', self.sample_port_config,
            '--var-json', 'MGMT_PORT'
        ]
        output = json.loads(self.run_script(argument))
        self.assertDictEqual(
            output, {'eth0': {'alias': 'Management1/1', 'admin_status': 'up'}})

    def test_device_metadata(self):
        argument = [
            '-m', self.sample_graph,
            '-p', self.sample_port_config,
            '--var-json', 'DEVICE_METADATA'
        ]
        out =(self.run_script(argument))
        print(out)
        output = json.loads(out)
        print(output['localhost'])
        self.assertDictEqual(output['localhost'], 
            {
                "region": "test",
                "cloudtype": "Public",
                "docker_routing_config_mode": "separated",
                "hostname": "str-sonic-sup00",
                "hwsku": "Sonic-supvisor-sku",
                "type": "SpineRouter",
                "synchronous_mode": "enable",
                "yang_config_validation": "disable",
                "chassis_hostname": "str-sonic",
                "deployment_id": "3",
                "cluster": "TestbedForstr-sonic",
                "subtype": "Supervisor",
                "switch_type": "fabric",
                "sub_role": "fabric",
                "max_cores": 64
            }
        )

    def test_device_metadata_for_namespace(self):
        argument = [
            '-m', self.sample_graph,
            '-n', 'asic0',
            '--var-json', 'DEVICE_METADATA'
        ]
        output = json.loads(self.run_script(argument))
        print(output['localhost'])
        self.assertDictEqual(output['localhost'], 
            {
                "region": "test",
                "cloudtype": None,
                "docker_routing_config_mode": "separated",
                "hostname": "str-sonic-sup00",
                "hwsku": "Sonic-supvisor-sku",
                "type": "SpineRouter",
                "synchronous_mode": "enable",
                "yang_config_validation": "disable",
                "chassis_hostname": "str-sonic",
                "deployment_id": "3",
                "cluster": "TestbedForstr-sonic",
                "sub_role": "Fabric",
                "asic_name": "asic0",
                "switch_type": "fabric",
                "max_cores": 64,
                "subtype": "Supervisor",
            }
        )

    
    def tearDown(self):
        os.environ['CFGGEN_UNIT_TESTING'] = ''
        os.environ['CFGGEN_UNIT_TESTING_TOPOLOGY'] = ''
        if os.path.exists(self.output_file):
            os.remove(self.output_file)


class TestPacketChassisSup(TestChassis):

    def setUp(self):
        super().setUp()
        self.test_data_dir = os.path.join(
            self.test_dir,  'chassis_data/packet_chassis_data')
        self.sample_graph = os.path.join(
            self.test_data_dir, 'packet_chassis_sup.xml')
        self.sample_port_config = os.path.join(
            self.test_data_dir, 'packet-chassis-port-config-1.ini')
        os.environ['CFGGEN_UNIT_TESTING'] = '2'
        os.environ["CFGGEN_UNIT_TESTING_TOPOLOGY"] = "multi_asic"

    def test_dummy_run(self):
        argument = []
        output = self.run_script(argument)
        self.assertEqual(output, '')

    def test_print_data(self):
        argument = [
            '-m', self.sample_graph, 
            '-p', self.sample_port_config,    
            '--print-data']
        output = self.run_script(argument)
        self.assertGreater(len(output.strip()), 0)

    def test_read_yaml(self):
        argument = ['-v', 'yml_item', '-y',
                    os.path.join(self.test_dir, 'test.yml')]
        output = yaml.safe_load(self.run_script(argument))
        self.assertListEqual(output, ['value1', 'value2'])

    def test_render_template(self):
        argument = ['-y', os.path.join(self.test_dir, 'test.yml'),
                    '-t', os.path.join(self.test_dir, 'test.j2')]
        output = self.run_script(argument)
        self.assertEqual(output.strip(), 'value1\nvalue2')

    def test_tacacs(self):
        argument = [
            '-m', self.sample_graph,
            '-p', self.sample_port_config,
            '--var-json', 'TACPLUS_SERVER'
        ]
        output = json.loads(self.run_script(argument))
        self.assertDictEqual(
            output, {'123.46.98.21': {'priority': '1', 'tcp_port': '49'}})
        # TACPLUS_SERVER not present in the asic configuration.
        argument = ['-m', self.sample_graph, '--var-json', 'TACPLUS_SERVER']

    def test_ntp(self):
        argument = [
            '-m', self.sample_graph,
            '-p', self.sample_port_config,
            '--var-json', 'NTP_SERVER'
        ]
        output = json.loads(self.run_script(argument))
        self.assertDictEqual(output, {'17.39.1.130': {'iburst': 'on'}, '17.39.1.129': {'iburst': 'on'}})
        # NTP data is present only in the host config
        argument = ['-m', self.sample_graph, '--var-json', 'NTP_SERVER']

    def test_mgmt_port(self):
        argument = [
            '-m', self.sample_graph,
            '-p', self.sample_port_config, 
            '--var-json', 'MGMT_PORT'
        ]
        output = json.loads(self.run_script(argument))
        self.assertDictEqual(
            output, {'eth0': {'alias': 'Management1/1', 'admin_status': 'up'}})

    def test_device_metadata(self):
        argument = [
            '-m', self.sample_graph,
            '-p', self.sample_port_config,
            '--var-json', 'DEVICE_METADATA'
        ]
        output = json.loads(self.run_script(argument))
        print(output['localhost'])
        self.assertDictEqual(output['localhost'], 
            {
                "region": "test",
                "cloudtype": "Public",
                "docker_routing_config_mode": "separated",
                "hostname": "str-sonic-sup00",
                "hwsku": "sonic-sup-sku",
                "type": "SpineRouter",
                "synchronous_mode": "enable",
                "yang_config_validation": "disable",
                "chassis_hostname": "str-sonic",
                "deployment_id": "3",
                "cluster": "TestbedForstr-sonic",
                "subtype": "Supervisor",
                "switch_type": "chassis-packet",
                "sub_role": "BackEnd",
                "max_cores": 64
            }
        )

    def test_device_metadata_for_namespace(self):
        argument = [
            '-m', self.sample_graph,
            '-n', 'asic0',
            '--var-json', 'DEVICE_METADATA'
        ]
        output = json.loads(self.run_script(argument))
        print(output['localhost'])
        self.assertDictEqual(output['localhost'], 
            {
                "region": "test",
                "cloudtype": None,
                "docker_routing_config_mode": "separated",
                "hostname": "str-sonic-sup00",
                "hwsku": "sonic-sup-sku",
                "type": "SpineRouter",
                "synchronous_mode": "enable",
                "yang_config_validation": "disable",
                "chassis_hostname": "str-sonic",
                "deployment_id": "3",
                "cluster": "TestbedForstr-sonic",
                "sub_role": "BackEnd",
                "subtype": "Supervisor",
                "asic_name": "asic0",
                "switch_type": "chassis-packet",
                "max_cores": 64
            }
        )

    def test_port(self):
        argument = [
            '-m', self.sample_graph,
            '-p', self.sample_port_config,
            '-n', 'asic0',
            '-v', "PORT[\'Ethernet-BP244\']"
        ]
        output = self.run_script(argument, ignore_warning=True)
        print(output)
        self.assertEqual(
           output.strip(),
           "{'lanes': '2828,2829', 'alias': 'Eth244-ASIC0', 'index': '122', 'speed': '100000', 'asic_port_name': 'Eth244-ASIC0', 'role': 'Int', 'fec': 'rs', 'description': 'Eth244-ASIC0', 'mtu': '9100', 'tpid': '0x8100', 'pfc_asym': 'off', 'admin_status': 'up'}"
        )

    def test_port_channel(self):
        argument = [
            '-m', self.sample_graph, 
            '-p', self.sample_port_config,
            '-n', 'asic0',
            '-v', "PORTCHANNEL[\'PortChannel5004\']"
        ]
        output = self.run_script(argument, ignore_warning=True)
        print(output)
        self.assertEqual(
            output.strip(),
            """{'min_links': '2', 'lacp_key': 'auto', 'mtu': '9100', 'tpid': '0x8100', 'admin_status': 'up'}""")
    
    def test_port_channel_member(self):
        argument = [
            '-m', self.sample_graph,
            '-p', self.sample_port_config,
            '-n', 'asic0',
            '-v', "PORTCHANNEL_MEMBER.keys()|list"
        ]
        output = self.run_script(argument, ignore_warning=True)
        print(output)
        self.assertEqual(
            utils.liststr_to_dict(output.strip()),
            utils.liststr_to_dict(
                "[('PortChannel5004', 'Ethernet-BP128'), "
                "('PortChannel5004', 'Ethernet-BP132'), "
                "('PortChannel5005', 'Ethernet-BP136'), "
                "('PortChannel5005', 'Ethernet-BP140'), "
                "('PortChannel5006', 'Ethernet-BP154'), "
                "('PortChannel5006', 'Ethernet-BP168'), "
                "('PortChannel5006', 'Ethernet-BP178'), "
                "('PortChannel5007', 'Ethernet-BP156'), "
                "('PortChannel5007', 'Ethernet-BP170'), "
                "('PortChannel5007', 'Ethernet-BP180'), "
                "('PortChannel5008', 'Ethernet-BP160'), "
                "('PortChannel5008', 'Ethernet-BP164'), "
                "('PortChannel5009', 'Ethernet-BP184'), "
                "('PortChannel5009', 'Ethernet-BP194'), "
                "('PortChannel5009', 'Ethernet-BP220'), "
                "('PortChannel5010', 'Ethernet-BP190'), "
                "('PortChannel5010', 'Ethernet-BP192'), "
                "('PortChannel5010', 'Ethernet-BP218')]"
            )
        )

    def tearDown(self):
        os.environ['CFGGEN_UNIT_TESTING'] = ''
        os.environ['CFGGEN_UNIT_TESTING_TOPOLOGY'] = ''
        if os.path.exists(self.output_file):
            os.remove(self.output_file)
