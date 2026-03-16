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
