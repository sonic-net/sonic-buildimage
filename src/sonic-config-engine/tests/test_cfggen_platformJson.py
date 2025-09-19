import ast
import json
import os
import subprocess
import sys
import ast
import tests.common_utils as utils

from unittest import TestCase
from portconfig import get_port_config, INTF_KEY

if sys.version_info.major == 3:
    from unittest import mock
else:
    import mock

# Global Variable
PLATFORM_OUTPUT_FILE = "platform_output.json"

class TestCfgGenPlatformJson(TestCase):

    def setUp(self):
        self.test_dir = os.path.dirname(os.path.realpath(__file__))
        self.script_file = [utils.PYTHON_INTERPRETTER, os.path.join(self.test_dir, '..', 'sonic-cfggen')]
        self.platform_sample_graph = os.path.join(self.test_dir, 'platform-sample-graph.xml')
        self.platform_json = os.path.join(self.test_dir, 'sample_platform.json')
        self.hwsku_json = os.path.join(self.test_dir, 'sample_hwsku.json')

    def run_script(self, argument, check_stderr=False):
        print('\n    Running sonic-cfggen ', argument)
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

    def test_print_data(self):
        argument = ['-m', self.platform_sample_graph, '-p', self.platform_json, '--print-data']
        output = self.run_script(argument)
        self.assertTrue(len(output.strip()) > 0)

    # Check whether all interfaces present or not as per platform.json
    def test_platform_json_interfaces_keys(self):
        argument = ['-m', self.platform_sample_graph, '-p', self.platform_json, '-S', self.hwsku_json, '-v', "PORT.keys()|list"]
        output = self.run_script(argument)
        self.maxDiff = None

        expected = [
            'Ethernet0', 'Ethernet4', 'Ethernet6', 'Ethernet8', 'Ethernet9', 'Ethernet10', 'Ethernet11', 'Ethernet12', 'Ethernet13', 'Ethernet14', 'Ethernet16',
            'Ethernet18', 'Ethernet19', 'Ethernet20', 'Ethernet24', 'Ethernet26', 'Ethernet28', 'Ethernet29', 'Ethernet30', 'Ethernet31', 'Ethernet32', 'Ethernet33',
            'Ethernet34', 'Ethernet36', 'Ethernet38', 'Ethernet39', 'Ethernet40', 'Ethernet44', 'Ethernet46', 'Ethernet48', 'Ethernet49', 'Ethernet50', 'Ethernet51',
            'Ethernet52', 'Ethernet53', 'Ethernet54', 'Ethernet56', 'Ethernet58', 'Ethernet59', 'Ethernet60', 'Ethernet64', 'Ethernet66', 'Ethernet68', 'Ethernet69',
            'Ethernet70', 'Ethernet71', 'Ethernet72', 'Ethernet73', 'Ethernet74', 'Ethernet76', 'Ethernet78', 'Ethernet79', 'Ethernet80', 'Ethernet84', 'Ethernet86',
            'Ethernet88', 'Ethernet89', 'Ethernet90', 'Ethernet91', 'Ethernet92', 'Ethernet93', 'Ethernet94', 'Ethernet96', 'Ethernet98', 'Ethernet99', 'Ethernet100',
            'Ethernet104', 'Ethernet106', 'Ethernet108', 'Ethernet109', 'Ethernet110', 'Ethernet111', 'Ethernet112', 'Ethernet113', 'Ethernet114', 'Ethernet116',
            'Ethernet118', 'Ethernet119', 'Ethernet120', 'Ethernet124', 'Ethernet126', 'Ethernet128', 'Ethernet132', 'Ethernet136', 'Ethernet137', 'Ethernet138',
            'Ethernet139', 'Ethernet140', 'Ethernet141', 'Ethernet142', 'Ethernet144'
            ]

        self.assertEqual(sorted(ast.literal_eval(output.strip())), sorted(expected))

    # Check specific Interface with it's proper configuration as per platform.json
    def test_platform_json_specific_ethernet_interfaces(self):

        argument = ['-m', self.platform_sample_graph, '-p', self.platform_json, '-S', self.hwsku_json, '-v', "PORT[\'Ethernet8\']"]
        output = self.run_script(argument)
        self.maxDiff = None
        expected = "{'index': '3', 'lanes': '8', 'description': 'Eth3/1', 'mtu': '9100', 'alias': 'Eth3/1', 'pfc_asym': 'off', 'speed': '25000', 'subport': '1', 'tpid': '0x8100'}"
        self.assertEqual(utils.to_dict(output.strip()), utils.to_dict(expected))

        argument = ['-m', self.platform_sample_graph, '-p', self.platform_json, '-S', self.hwsku_json, '-v', "PORT[\'Ethernet112\']"]
        output = self.run_script(argument)
        self.maxDiff = None
        expected = "{'index': '29', 'lanes': '112', 'description': 'Eth29/1', 'mtu': '9100', 'alias': 'Eth29/1', 'pfc_asym': 'off', 'speed': '25000', 'subport': '1', 'tpid': '0x8100'}"
        self.assertEqual(utils.to_dict(output.strip()), utils.to_dict(expected))

        argument = ['-m', self.platform_sample_graph, '-p', self.platform_json, '-S', self.hwsku_json, '-v', "PORT[\'Ethernet4\']"]
        output = self.run_script(argument)
        self.maxDiff = None
        expected = "{'index': '2', 'lanes': '4,5', 'description': 'Eth2/1', 'admin_status': 'up', 'mtu': '9100', 'alias': 'Eth2/1', 'pfc_asym': 'off', 'speed': '50000', 'subport': '1', 'tpid': '0x8100'}"
        print(output.strip())
        self.assertEqual(utils.to_dict(output.strip()), utils.to_dict(expected))

    # Check all Interface with it's proper configuration as per platform.json
    def test_platform_json_all_ethernet_interfaces(self):
        argument = ['-m', self.platform_sample_graph, '-p', self.platform_json, '-S', self.hwsku_json, '-v', "PORT"]
        output = self.run_script(argument)
        self.maxDiff = None

        sample_file = os.path.join(self.test_dir, 'sample_output', PLATFORM_OUTPUT_FILE)
        fh = open(sample_file, 'rb')
        fh_data = json.load(fh)
        fh.close()

        output_dict = ast.literal_eval(output.strip())
        expected = ast.literal_eval(json.dumps(fh_data))
        self.assertDictEqual(output_dict, expected)

    @mock.patch('portconfig.readJson', mock.MagicMock(return_value={INTF_KEY:{}}))
    @mock.patch('os.path.isfile', mock.MagicMock(return_value=True))
    def test_platform_json_no_interfaces(self):
        (ports, _, _) = get_port_config(port_config_file=self.platform_json)
        self.assertNotEqual(ports, None)
        self.assertEqual(ports, {})

    # Check that FEC 'rs' is properly set for lanes with speed >= 50G per lane
    def test_fec_rs(self):
        # Ethernet0 is 1x100G
        argument = ['-m', self.platform_sample_graph, '-p', self.platform_json, '-S', self.hwsku_json, '-v', "PORT[\'Ethernet0\']"]
        output = self.run_script(argument)
        port_config = utils.to_dict(output.strip())
        self.assertIn('fec', port_config)
        self.assertEqual(port_config['fec'], 'rs')

        # Ethernet4 is 2x50G
        argument = ['-m', self.platform_sample_graph, '-p', self.platform_json, '-S', self.hwsku_json, '-v', "PORT[\'Ethernet4\']"]
        output = self.run_script(argument)
        port_config = utils.to_dict(output.strip())
        self.assertNotIn('fec', port_config)

    # Check FEC logic with custom platform.json
    def test_fec_rs_custom(self):
        test_platform_json = {
            "interfaces": {
                "Ethernet200": {
                    "index": "50,50,50,50,50,50,50,50",
                    "lanes": "200,201,202,203,204,205,206,207",
                    "breakout_modes": {
                        "8x800G": ["Eth50/1", "Eth50/2", "Eth50/3", "Eth50/4", "Eth50/5", "Eth50/6", "Eth50/7", "Eth50/8"]
                    }
                },
                "Ethernet208": {
                    "index": "51,51,51,51",
                    "lanes": "208,209,210,211",
                    "breakout_modes": {
                        "4x25G": ["Eth51/1", "Eth51/2", "Eth51/3", "Eth51/4"]
                    }
                }
            }
        }

        test_hwsku_json = {
            "interfaces": {
                "Ethernet200": {"default_brkout_mode": "8x800G"},
                "Ethernet208": {"default_brkout_mode": "4x25G"},
            }
        }

        with mock.patch('portconfig.readJson') as mock_read_json:
            def side_effect(filename):
                if 'platform' in filename:
                    return test_platform_json
                elif 'hwsku' in filename:
                    return test_hwsku_json
                return None

            mock_read_json.side_effect = side_effect

            from portconfig import get_child_ports
            ports = get_child_ports("Ethernet200", "8x800G", "test_platform.json")
            self.assertIn('fec', ports['Ethernet200'])
            self.assertEqual(ports['Ethernet200']['fec'], 'rs')

            ports = get_child_ports("Ethernet208", "4x25G", "test_platform.json")
            self.assertNotIn('fec', ports['Ethernet208'])

    # Check FEC logic for edge cases around the 50G per lane threshold
    def test_fec_rs_for_edge_cases(self):
        test_platform_json = {
            "interfaces": {
                "Ethernet200": {
                    "index": "50,50",
                    "lanes": "200,201",
                    "breakout_modes": {
                        "1x100G": ["Eth50/1"], 
                        "2x50G": ["Eth50/1", "Eth50/2"]
                    }
                },
                "Ethernet204": {
                    "index": "51,51",
                    "lanes": "204,205",
                    "breakout_modes": {
                        "1x102G": ["Eth51/1"]
                    }
                },
                "Ethernet208": {
                    "index": "52",
                    "lanes": "208",
                    "breakout_modes": {
                        "1x50G": ["Eth52/1"]
                    }
                },
                "Ethernet212": {
                    "index": "53",
                    "lanes": "212",
                    "breakout_modes": {
                        "1x49G": ["Eth53/1"]
                    }
                }
            }
        }

        test_hwsku_json = {
            "interfaces": {
                "Ethernet200": {"default_brkout_mode": "1x100G"},
                "Ethernet204": {"default_brkout_mode": "1x102G"},
                "Ethernet208": {"default_brkout_mode": "1x50G"},
                "Ethernet212": {"default_brkout_mode": "1x49G"}
            }
        }

        with mock.patch('portconfig.readJson') as mock_read_json:
            def side_effect(filename):
                if 'platform' in filename:
                    return test_platform_json
                elif 'hwsku' in filename:
                    return test_hwsku_json
                return None

            mock_read_json.side_effect = side_effect

            from portconfig import get_child_ports
            ports = get_child_ports("Ethernet200", "1x100G", "test_platform.json")
            self.assertIn('fec', ports['Ethernet200'])
            self.assertEqual(ports['Ethernet200']['fec'], 'rs')

            ports = get_child_ports("Ethernet204", "1x102G", "test_platform.json")
            self.assertIn('fec', ports['Ethernet204'])
            self.assertEqual(ports['Ethernet204']['fec'], 'rs')

            ports = get_child_ports("Ethernet208", "1x50G", "test_platform.json")
            self.assertIn('fec', ports['Ethernet208'])
            self.assertEqual(ports['Ethernet208']['fec'], 'rs')

            ports = get_child_ports("Ethernet212", "1x49G", "test_platform.json")
            self.assertNotIn('fec', ports['Ethernet212'])
