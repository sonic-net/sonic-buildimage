import ast
import io
import json
import os
import runpy
import shutil
import subprocess
import tempfile
import ast
import tests.common_utils as utils

from contextlib import contextmanager
from unittest import TestCase, mock
from portconfig import get_breakout_mode, get_port_config, INTF_KEY

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

        from portconfig import get_child_ports

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

        minimum = get_child_ports("Ethernet192", None, "test_platform.json", select_minimum=True)
        self.assertEqual(minimum["Ethernet192"]["alias"], "etp25")
        self.assertEqual(minimum["Ethernet192"]["speed"], "100000")
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

    def test_minimum_breakout_is_opt_in(self):
        with port_config_env(self.platform_json):
            self.assertEqual(get_port_config(hwsku='TestHwsku'), ({}, {}, {}))
            ports, _, _ = get_port_config(hwsku='TestHwsku', allow_minimum_breakout=True)
            self.assertEqual(len(ports), 37)
            self.assertEqual(ports['Ethernet0'], {
                'alias': 'Eth1', 'lanes': '0,1,2,3', 'speed': '100000', 'index': '1', 'subport': '0',
            })
            self.assertIsNone(get_breakout_mode(hwsku='TestHwsku'))

            result = _run_cfggen_cli_json(['-k', 'TestHwsku', '-p', self.platform_json])
            self.assertEqual(result['PORT'], ports)
            self.assertNotIn('BREAKOUT_CFG', result)

    def test_config_db_source(self):
        ports = {
            'Ethernet0': {'alias': 'saved/1', 'asic_port_name': 'Eth0', 'lanes': '40,41', 'speed': '50000'},
            'Ethernet4': {'lanes': '42', 'speed': '25000'},
        }
        aliases = {'saved/1': 'Ethernet0', 'Eth0': 'Ethernet0', 'Ethernet4': 'Ethernet4'}
        config_db = mock.Mock()
        config_db.get_table.side_effect = {'PORT': ports, 'BREAKOUT_CFG': {}}.__getitem__
        for hwsku in (None, 'TestHwsku', 'OtherHwsku'):
            with self.subTest(hwsku=hwsku), port_config_env(config_db=config_db), \
                    mock.patch('portconfig.db_connect_configdb', return_value=config_db) as connect:
                expected = ({}, {}, {}) if hwsku == 'OtherHwsku' else (ports, aliases, {'saved/1': 'Eth0'})
                self.assertEqual(get_port_config(hwsku=hwsku, asic_name='asic0'), expected)
                self.assertIsNone(get_breakout_mode(hwsku=hwsku, asic_name='asic0'))
                self.assertEqual(connect.call_args_list, [mock.call('asic0')] * 2)

    def test_config_db_file_fallback_and_override(self):
        saved = {'Ethernet999': {'alias': 'saved'}}
        for hwsku_file, saved_ports, db_hwsku, port_file in (
                (self.hwsku_json, {}, 'TestHwsku', None),
                (None, saved, 'OtherHwsku', None),
                (None, saved, 'TestHwsku', self.platform_json),
                (self.hwsku_json, saved, 'TestHwsku', self.platform_json)):
            config_db = mock.Mock()
            config_db.get_table.side_effect = {
                'PORT': saved_ports, 'BREAKOUT_CFG': {'Ethernet999': {'brkout_mode': '1x100G'}}}.__getitem__
            kwargs = {'hwsku': 'TestHwsku', 'port_config_file': port_file}
            with self.subTest(hwsku_file=hwsku_file, db_hwsku=db_hwsku, port_file=port_file), \
                    port_config_env(None if port_file else self.platform_json, hwsku_file, config_db, db_hwsku):
                result = (get_port_config(allow_minimum_breakout=True, **kwargs)[0], get_breakout_mode(**kwargs))
                with mock.patch('portconfig.db_connect_configdb', return_value=None):
                    expected = (get_port_config(allow_minimum_breakout=True, **kwargs)[0],
                                get_breakout_mode(**kwargs))
                self.assertEqual(result, expected)
                if port_file or db_hwsku != 'TestHwsku':
                    config_db.get_table.assert_not_called()

    def test_external_selection_overrides_installed_selection_and_db(self):
        config_db = mock.Mock()
        with port_config_env(self.platform_json, self.hwsku_json, config_db=config_db) as directory:
            selection = os.path.join(directory, 'selection.json')
            with open(selection, 'w') as config:
                json.dump({'interfaces': {
                    'Ethernet4': {'default_brkout_mode': '2x50G', 'autoneg': 'off'}}}, config)
            ports, _, _ = get_port_config(hwsku='TestHwsku', hwsku_config_file=selection)
            self.assertEqual(set(ports), {'Ethernet4', 'Ethernet6'})
            self.assertTrue(all(port['autoneg'] == 'off' for port in ports.values()))
            self.assertEqual(get_breakout_mode(hwsku='TestHwsku', hwsku_config_file=selection),
                             {'Ethernet4': {'brkout_mode': '2x50G'}})

            result = _run_cfggen_cli_json(['-k', 'TestHwsku', '-p', self.platform_json, '-S', selection])
            self.assertEqual(result['PORT'], ports)
            self.assertEqual(result['BREAKOUT_CFG'], {'Ethernet4': {'brkout_mode': '2x50G'}})
            config_db.get_table.assert_not_called()
