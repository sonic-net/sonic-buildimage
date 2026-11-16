import ast
import io
import json
import os
import runpy
import shutil
import subprocess
import tempfile
import tests.common_utils as utils

from contextlib import contextmanager
from unittest import TestCase, mock
from portconfig import get_child_ports, get_port_config, INTF_KEY

# Global Variable
PLATFORM_OUTPUT_FILE = "platform_output.json"


@contextmanager
def port_config_env(platform_json=None, hwsku_json=None, config_db=None, db_hwsku='TestHwsku'):
    """Isolate port-file discovery and CONFIG_DB state for reader and CLI tests."""
    with tempfile.TemporaryDirectory() as directory, \
            mock.patch.dict(os.environ, {'CFGGEN_UNIT_TESTING': '0'}), \
            mock.patch('portconfig.db_connect_configdb', return_value=config_db), \
            mock.patch('portconfig.device_info.get_localhost_info', return_value=db_hwsku), \
            mock.patch('portconfig.device_info.get_platform', return_value='TestPlatform'), \
            mock.patch('portconfig.device_info.get_path_to_platform_dir', return_value=directory), \
            mock.patch('sonic_py_common.multi_asic.get_asic_conf_file_path', return_value=None), \
            mock.patch.multiple('portconfig', HWSKU_ROOT_PATH=directory, PLATFORM_ROOT_PATH=directory,
                                PLATFORM_ROOT_PATH_DOCKER=directory, SONIC_ROOT_PATH=directory):
        for source, filename in ((platform_json, 'platform.json'), (hwsku_json, 'hwsku.json')):
            if source is not None:
                shutil.copyfile(source, os.path.join(directory, filename))
        yield directory


def _run_cfggen_cli_json(args):
    script = os.path.join(os.path.dirname(__file__), '..', 'sonic-cfggen')
    with mock.patch('sys.argv', [script] + args + ['--print-data']), \
            mock.patch('swsscommon.swsscommon.SonicDBConfig.isInit', return_value=True), \
            mock.patch('sys.stdout', new_callable=io.StringIO) as output:
        runpy.run_path(script, run_name='__main__')
    return json.loads(output.getvalue())


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

    # Check all Interface with it's proper configuration as per platform.json
    def test_platform_json_all_ethernet_interfaces(self):
        argument = ['-m', self.platform_sample_graph, '-p', self.platform_json, '-S', self.hwsku_json, '-v', "PORT"]
        output = self.run_script(argument)
        self.maxDiff = None

        sample_file = os.path.join(self.test_dir, 'sample_output', PLATFORM_OUTPUT_FILE)
        with open(sample_file) as config:
            self.assertDictEqual(ast.literal_eval(output.strip()), json.load(config))

    @mock.patch('portconfig.readJson', mock.MagicMock(return_value={INTF_KEY:{}}))
    @mock.patch('os.path.isfile', mock.MagicMock(return_value=True))
    def test_platform_json_no_interfaces(self):
        (ports, _, _) = get_port_config(port_config_file=self.platform_json)
        self.assertEqual(ports, {})

    @mock.patch('portconfig.readJson')
    def test_fec_rs_for_edge_cases(self, mock_read_json):
        for mode, lane_count, port_count, fec in (
                ('8x800G', 8, 8, True),
                ('4x25G', 4, 4, False),
                ('1x100G', 2, 1, True),
                ('1x102G', 2, 1, True),
                ('1x50G', 1, 1, True),
                ('1x49G', 1, 1, False)):
            with self.subTest(mode=mode):
                mock_read_json.return_value = {'interfaces': {'Ethernet200': {
                    'index': ','.join(['50'] * lane_count),
                    'lanes': ','.join(str(200 + lane) for lane in range(lane_count)),
                    'breakout_modes': {mode: [f'Eth50/{port + 1}' for port in range(port_count)]}
                }}}
                ports = get_child_ports('Ethernet200', mode, 'test_platform.json')
                self.assertEqual(len(ports), port_count)
                for port in ports.values():
                    if fec:
                        self.assertEqual(port['fec'], 'rs')
                    else:
                        self.assertNotIn('fec', port)

    @mock.patch('portconfig.readJson')
    def test_subport_for_mixed_width_breakout(self, mock_read_json):
        test_platform_json = {
            "interfaces": {
                "Ethernet192": {
                    "index": "25,25,25,25,25,25,25,25",
                    "lanes": "192,193,194,195,196,197,198,199",
                    "breakout_modes": {
                        "8x100G[50G]": [
                            "etp25a", "etp25b", "etp25c", "etp25d",
                            "etp25e", "etp25f", "etp25g", "etp25h"
                        ],
                        "1x50G(2)": ["etp25a"],
                        "1x100G[40G]": ["etp25a"],
                        "1x100G": "etp25",
                        "4x25G(4)[10G,1G]": ["etp25a", "etp25b", "etp25c", "etp25d"],
                        "4x100G[50G](4)+1x400G[200G](4)": [
                            "etp25a", "etp25b", "etp25c", "etp25d", "etp25e"
                        ],
                        "1x400G[200G](4)+4x100G[50G](4)": [
                            "etp25a", "etp25b", "etp25c", "etp25d", "etp25e"
                        ],
                        "2x400G[200G](4)+1x800G[400G](4)": [
                            "etp25a", "etp25b", "etp25c"
                        ]
                    }
                }
            }
        }
        mock_read_json.return_value = test_platform_json

        expected_subports = {
            "8x100G[50G]": ["1", "2", "3", "4", "5", "6", "7", "8"],
            "1x100G": ["0"],
            "4x25G(4)[10G,1G]": ["1", "2", "3", "4"],
            "4x100G[50G](4)+1x400G[200G](4)": ["1", "2", "3", "4", "2"],
            "1x400G[200G](4)+4x100G[50G](4)": ["1", "5", "6", "7", "8"],
            # Exact mode from issue #29427 (Mellanox-SN6600)
            "2x400G[200G](4)+1x800G[400G](4)": ["1", "2", "2"]
        }

        for breakout_mode, expected in expected_subports.items():
            ports = get_child_ports("Ethernet192", breakout_mode, "test_platform.json")
            port_configs = [ports[name] for name in sorted(
                ports, key=lambda name: int(name.replace("Ethernet", "")))]
            self.assertEqual(
                [port["subport"] for port in port_configs],
                expected
            )
            self.assertEqual(sum(len(port["lanes"].split(",")) for port in port_configs),
                             4 if breakout_mode == "4x25G(4)[10G,1G]" else 8)

            for port in port_configs:
                lanes = [int(lane) for lane in port["lanes"].split(",")]
                subport_lane_offset = max(int(port["subport"]) - 1, 0) * len(lanes)
                self.assertEqual(subport_lane_offset, lanes[0] - 192)

        modes = test_platform_json["interfaces"]["Ethernet192"]["breakout_modes"]
        for entries in (list(modes.items()), list(reversed(list(modes.items())))):
            with mock.patch.dict(modes, dict(entries), clear=True):
                minimum = get_child_ports("Ethernet192", None, "test_platform.json", select_minimum=True)
                self.assertEqual(minimum["Ethernet192"]["alias"], "etp25a")
                self.assertEqual(minimum["Ethernet192"]["speed"], "100000")
        with mock.patch.dict(modes, {"1x100G": "etp25"}, clear=True):
            minimum = get_child_ports("Ethernet192", None, "test_platform.json", select_minimum=True)
            self.assertEqual(minimum["Ethernet192"]["alias"], "etp25")
        with mock.patch.dict(test_platform_json["interfaces"]["Ethernet192"]["breakout_modes"],
                             {"2x100G": ["etp25a", "etp25b"]}, clear=True):
            with self.assertRaisesRegex(RuntimeError, r"explicit selection.*Ethernet192.*2x100G"):
                get_child_ports("Ethernet192", None, "test_platform.json", select_minimum=True)

        for field, interface in (("lanes", "Ethernet192"), ("index", "Ethernet192"), (None, "Port192")):
            properties = {key: value for key, value in test_platform_json["interfaces"]["Ethernet192"].items()
                          if key != field}
            with self.subTest(field=field, interface=interface), \
                    mock.patch.dict(test_platform_json["interfaces"], {interface: properties}, clear=True):
                with self.assertRaisesRegex(RuntimeError, rf"{interface}.*Supported modes:.*1x100G"):
                    get_child_ports(interface, "1x100G", "test_platform.json")

    def test_invalid_selection(self):
        mode_error = r"Ethernet20.*Supported modes:"
        with port_config_env(self.platform_json) as directory:
            selection = os.path.join(directory, 'hwsku.json')
            for interfaces, error in (
                    ({'Ethernet20': {}}, mode_error),
                    ({'Ethernet20': None}, mode_error),
                    ({'Ethernet20': {'default_brkout_mode': None}}, mode_error),
                    ({'Ethernet20': {'default_brkout_mode': 42}}, mode_error),
                    ({'Ethernet20': {'default_brkout_mode': 'invalid'}}, mode_error),
                    ({'Ethernet20': {'default_brkout_mode': '1xG[40G]'}}, mode_error),
                    ({'Ethernet20': {'default_brkout_mode': '3x25G'}}, mode_error),
                    ({'Ethernet20': {'default_brkout_mode': '1x100G[40G]'},
                      'Ethernet999': {'default_brkout_mode': '1x100G'}}, r"Unknown interfaces.*Ethernet999")):
                with self.subTest(interfaces=interfaces):
                    with open(selection, 'w') as config:
                        json.dump({'interfaces': interfaces}, config)
                    with self.assertRaisesRegex(RuntimeError, error):
                        get_port_config(hwsku='TestHwsku', hwsku_config_file=selection)
                    if 'Ethernet999' in interfaces:
                        self.assertEqual(set(get_port_config(hwsku='TestHwsku')[0]), {'Ethernet20'})

    def test_port_source_precedence(self):
        saved = {'Ethernet20': {'alias': 'saved', 'lanes': '80,81,82,83', 'speed': '100000'}}
        for rows, db_hwsku, use_db in ((saved, 'TestHwsku', True),
                                      (saved, 'OtherHwsku', False),
                                      ({}, 'TestHwsku', False)):
            config_db = mock.Mock()
            config_db.get_table.return_value = rows
            with self.subTest(rows=rows, db_hwsku=db_hwsku), \
                    port_config_env(self.platform_json, config_db=config_db, db_hwsku=db_hwsku):
                ports, _, _ = get_port_config(hwsku='TestHwsku')
                self.assertEqual(ports['Ethernet20']['lanes'],
                                 '80,81,82,83' if use_db else '20,21,22,23')
                direct = _run_cfggen_cli_json(['-k', 'TestHwsku'])
                self.assertEqual(direct['PORT']['Ethernet20']['lanes'], '20,21,22,23')
                self.assertNotIn('BREAKOUT_CFG', direct)

    def test_cli_breakout_metadata(self):
        for installed in (False, True):
            with self.subTest(installed=installed), port_config_env(
                    self.platform_json, self.hwsku_json if installed else None):
                result = _run_cfggen_cli_json(['-k', 'TestHwsku'])
                self.assertEqual('BREAKOUT_CFG' in result, installed)
                self.assertEqual(
                    _run_cfggen_cli_json(['-p', self.platform_json, '-S', self.hwsku_json]), {})
