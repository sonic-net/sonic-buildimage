import ast
import json
import os
import subprocess
import sys
import tempfile
import ast
import tests.common_utils as utils

from unittest import TestCase
from portconfig import get_breakout_mode, get_port_config, INTF_KEY

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

    def _port_config(self, platform=False, hwsku=False, config_db=None, db_hwsku='TestHwsku', **kwargs):
        with mock.patch('portconfig.db_connect_configdb', return_value=config_db), \
                mock.patch('portconfig.device_info.get_localhost_info', return_value=db_hwsku), \
                mock.patch('portconfig.device_info.get_path_to_port_config_file',
                           return_value=self.platform_json if platform else None), \
                mock.patch('portconfig.get_hwsku_file_name',
                           return_value=self.hwsku_json if hwsku else None):
            return (get_port_config(hwsku='TestHwsku', **kwargs)[0],
                    get_breakout_mode(hwsku='TestHwsku', **kwargs))

    def _port_config_from_data(self, interfaces, selection=None):
        files = {self.platform_json: {'interfaces': interfaces}}
        if selection is not None:
            files[self.hwsku_json] = {'interfaces': selection}
        with mock.patch('portconfig.readJson', side_effect=files.__getitem__), \
                mock.patch('portconfig.device_info.get_sonic_version_info', return_value={}):
            return self._port_config(platform=True, hwsku=selection is not None)

    def _config_db(self, ports, breakout=None):
        config_db = mock.Mock()
        config_db.get_table.side_effect = {'PORT': ports, 'BREAKOUT_CFG': breakout or {}}.__getitem__
        return config_db

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

            for port in port_configs:
                lanes = [int(lane) for lane in port["lanes"].split(",")]
                subport_lane_offset = (
                    (int(port["subport"]) - 1) * len(lanes)
                )
                self.assertEqual(subport_lane_offset, lanes[0] - 192)

    def test_no_hwsku_or_platform_json(self):
        self.assertEqual(self._port_config(), ({}, None))

    def test_hwsku_without_platform_json(self):
        self.assertEqual(self._port_config(hwsku=True), ({}, None))

    def test_platform_without_hwsku_json(self):
        ports, breakout = self._port_config(platform=True)
        self.assertEqual(len(ports), 37)
        self.assertEqual(ports['Ethernet0'], {
            'alias': 'Eth1', 'lanes': '0,1,2,3', 'speed': '100000', 'index': '1', 'subport': '0',
        })
        self.assertIsNone(breakout)

    def test_platform_and_hwsku_json(self):
        ports, breakout = self._port_config(platform=True, hwsku=True)
        self.assertEqual(len(ports), 91)
        self.assertEqual(ports['Ethernet4'], {
            'alias': 'Eth2/1', 'lanes': '4,5', 'speed': '50000', 'index': '2', 'subport': '1',
        })
        self.assertEqual(len(breakout), 91)
        self.assertEqual(breakout['Ethernet4'], {'brkout_mode': '2x50G'})

    def test_config_db_ports_take_priority(self):
        ports = {
            'Ethernet0': {'alias': 'saved/1', 'asic_port_name': 'Eth0', 'lanes': '40,41', 'speed': '50000'},
            'Ethernet4': {'lanes': '42', 'speed': '25000'},
        }
        aliases = {'saved/1': 'Ethernet0', 'Eth0': 'Ethernet0', 'Ethernet4': 'Ethernet4'}
        config_db = mock.Mock()
        with mock.patch('portconfig.db_connect_configdb', return_value=config_db) as connect, \
                mock.patch('portconfig.device_info.get_localhost_info', return_value='TestHwsku'), \
                mock.patch('portconfig.device_info.get_path_to_port_config_file') as discover, \
                mock.patch('portconfig.get_hwsku_file_name') as find_hwsku:
            for breakout in ({'Ethernet0': {'brkout_mode': '2x50G'}}, {}):
                config_db.get_table.side_effect = {'PORT': ports, 'BREAKOUT_CFG': breakout}.__getitem__
                for hwsku, namespace in ((None, None), ('TestHwsku', None), (None, 'asic0'), ('TestHwsku', 'asic0')):
                    with self.subTest(hwsku=hwsku, namespace=namespace, breakout=breakout):
                        connect.reset_mock()
                        self.assertEqual(get_port_config(hwsku=hwsku, asic_name=namespace),
                                         (ports, aliases, {'saved/1': 'Eth0'}))
                        self.assertEqual(get_breakout_mode(hwsku=hwsku, asic_name=namespace), breakout or None)
                        self.assertEqual(connect.call_args_list, [mock.call(namespace), mock.call(namespace)])
            discover.assert_not_called()
            find_hwsku.assert_not_called()

    def test_config_db_falls_back_to_files(self):
        for db_hwsku, saved_ports in (('TestHwsku', {}), ('OtherHwsku', {'Ethernet999': {'alias': 'saved'}})):
            config_db = self._config_db(saved_ports, {'Ethernet999': {'brkout_mode': '1x100G'}})
            for platform, hwsku in ((False, False), (False, True), (True, False), (True, True)):
                with self.subTest(db_hwsku=db_hwsku, platform=platform, hwsku=hwsku):
                    expected = self._port_config(platform=platform, hwsku=hwsku)
                    self.assertEqual(self._port_config(
                        platform=platform, hwsku=hwsku, config_db=config_db, db_hwsku=db_hwsku), expected)
            if db_hwsku != 'TestHwsku':
                config_db.get_table.assert_not_called()

    def test_explicit_platform_overrides_config_db(self):
        config_db = self._config_db({'Ethernet999': {'alias': 'saved'}})
        for hwsku in (False, True):
            with self.subTest(hwsku=hwsku):
                expected = self._port_config(platform=True, hwsku=hwsku)
                self.assertEqual(self._port_config(
                    hwsku=hwsku, config_db=config_db, port_config_file=self.platform_json), expected)
        config_db.get_table.assert_not_called()

    def test_external_selection_overrides_installed_selection_and_db(self):
        config_db = self._config_db({'Ethernet999': {'alias': 'saved'}})
        with tempfile.NamedTemporaryFile(mode='w', suffix='.json', delete=False) as selection:
            self.addCleanup(os.remove, selection.name)
            json.dump({'interfaces': {'Ethernet4': {'default_brkout_mode': '2x50G'}}}, selection)
        ports, breakout = self._port_config(
            platform=True, hwsku=True, config_db=config_db, hwsku_config_file=selection.name)
        self.assertEqual(set(ports), {'Ethernet4', 'Ethernet6'})
        self.assertEqual(breakout, {'Ethernet4': {'brkout_mode': '2x50G'}})
        config_db.get_table.assert_not_called()

    def test_string_aliases(self):
        mode = '1x25G[10G]'
        interfaces = {'Ethernet0': {
            'index': '1', 'lanes': '49', 'breakout_modes': {mode: 'Eth1'},
        }}
        for selection in (None, {'Ethernet0': {'default_brkout_mode': mode}}):
            with self.subTest(selection=selection):
                ports, breakout = self._port_config_from_data(interfaces, selection)
                self.assertEqual(ports, {'Ethernet0': {
                    'alias': 'Eth1', 'lanes': '49', 'speed': '25000', 'index': '1', 'subport': '0',
                }})
                self.assertEqual(breakout, {'Ethernet0': {'brkout_mode': mode}} if selection else None)

    def test_partial_lane_selection(self):
        mode = '4x25G(4)[10G,1G]'
        ports, breakout = self._port_config_from_data(
            {'Ethernet0': {
                'index': '1,1,1,1,1,1,1,1', 'lanes': '0,1,2,3,4,5,6,7',
                'breakout_modes': {mode: ['Eth1/1', 'Eth1/2', 'Eth1/3', 'Eth1/4']},
            }},
            {'Ethernet0': {'default_brkout_mode': mode, 'autoneg': 'off'}})
        self.assertEqual(len(ports), 4)
        for name, alias, lane, subport in [
                ('Ethernet0', 'Eth1/1', '0', '1'), ('Ethernet1', 'Eth1/2', '1', '2'),
                ('Ethernet2', 'Eth1/3', '2', '3'), ('Ethernet3', 'Eth1/4', '3', '4')]:
            self.assertEqual(ports[name], {
                'alias': alias, 'lanes': lane, 'speed': '25000', 'index': '1',
                'subport': subport, 'autoneg': 'off',
            })
        self.assertEqual(breakout, {'Ethernet0': {'brkout_mode': mode}})

    def test_mixed_width_selection(self):
        mode = '2x400G[200G](4)+1x800G[400G](4)'
        ports, breakout = self._port_config_from_data(
            {'Ethernet0': {
                'index': '1,1,1,1,1,1,1,1', 'lanes': '0,1,2,3,4,5,6,7',
                'breakout_modes': {mode: ['Eth1/1', 'Eth1/2', 'Eth1/3']},
            }},
            {'Ethernet0': {'default_brkout_mode': mode, 'autoneg': 'off'}})
        self.assertEqual(len(ports), 3)
        for name, alias, lanes, speed, subport in [
                ('Ethernet0', 'Eth1/1', '0,1', '400000', '1'),
                ('Ethernet2', 'Eth1/2', '2,3', '400000', '2'),
                ('Ethernet4', 'Eth1/3', '4,5,6,7', '800000', '2')]:
            self.assertEqual(ports[name], {
                'alias': alias, 'lanes': lanes, 'speed': speed, 'index': '1',
                'subport': subport, 'autoneg': 'off', 'fec': 'rs',
            })
        self.assertEqual(breakout, {'Ethernet0': {'brkout_mode': mode}})

    def test_minimum_chooses_fastest_single_port_mode(self):
        ports, breakout = self._port_config_from_data({'Ethernet1': {
            'index': '0,0,0,0', 'lanes': '5,6,7,8',
            'breakout_modes': {
                '1x50G(2)': ['Eth1/1'],
                '2x200G': ['Eth1/1', 'Eth1/2'],
                '1x100G[40G]': ['Eth1/1'],
                '1x100G': ['Eth1'],
            },
        }})
        self.assertEqual(ports, {'Ethernet1': {
            'alias': 'Eth1', 'lanes': '5,6,7,8', 'speed': '100000', 'index': '0', 'subport': '0',
        }})
        self.assertIsNone(breakout)
