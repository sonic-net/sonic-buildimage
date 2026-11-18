"""Focused rejection tests for the installed DASH engine evidence boundary."""
import copy
import importlib.util
import json
from pathlib import Path
import sys
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch

W = Path(__file__).resolve().parents[3]
HERE = Path(__file__).parent
sys.path.insert(0, str(W))
sys.path.insert(0, str(W / 'tools/bazel/oci'))


def load(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


loaded = load('engine_loaded', W / 'tools/bazel/ci/dash_engine_loaded.py')
loaded.ROOT = W
probe = load('engine_probe', W / 'platform/vs/docker-dash-engine/bazel/runtime_probe.py')


def receipt(debug=False):
    return {'status': 'passed', 'architecture': 'amd64', 'variant': 'debug' if debug else 'runtime',
        'source_elf_count': 2, 'loader_checks': 2, 'debug_checks': [
            {'path': p, 'symbol': s, 'without_symbols': {'found': False}, 'with_symbols': {'found': True}}
            for p, s in [('usr/bin/simple_switch_grpc', 'main'), ('usr/lib/python3/dist-packages/_nnpy.so', '_cffi_f_nn_socket')]
        ] + [{'path': 'usr/bin/pi_CLI_dummy', 'symbol': 'main',
              'debug_forwarding': {'provider': 'usr/lib/x86_64-linux-gnu/libpicli.so.0', 'symbol': 'main', 'source': 'main.c'},
              'without_symbols': {'found': False}, 'with_symbols': {'found': True, 'function': 'main',
              'source': 'main.c', 'line': 391, 'type': 'int (int, char **)'}}] if debug else [], 'startup': {'entrypoint': '/usr/bin/start.sh', 'stable_installed_process': True,
            'interfaces_up': ['eth1', 'eth2'], 'switch_arguments': ['simple_switch_grpc', '--interface', '0@eth1',
            '--interface', '1@eth2', '--log-console', '--no-p4'], 'p4runtime': {'api_version': '1.3.0',
            'device_id': 0, 'primary_arbitration': 'passed', 'no_pipeline_get_config': 'FAILED_PRECONDITION'},
            'nanomsg_clients': {'bm_nanomsg_events': {'packet_in_decoded': True, 'config_change_observed': True},
                'bm_p4dbg': {'attach_detach': True, 'request_types': [18, 19]}}}}


class LoadedTests(unittest.TestCase):
    def parse(self, value, debug=False):
        return loaded.parse_probe(json.dumps(value), debug, 2)

    def test_complete_runtime_and_debug_receipts(self):
        for debug in (False, True):
            self.assertEqual(self.parse(receipt(debug), debug), receipt(debug))

    def test_wrong_architecture(self):
        r = receipt(); r['architecture'] = 'arm64'
        with self.assertRaises(ValueError): self.parse(r)

    def test_missing_loader_check(self):
        r = receipt(); r['loader_checks'] = 1
        with self.assertRaises(ValueError): self.parse(r)

    def test_changed_original_startup(self):
        r = receipt(); r['startup']['switch_arguments'].append('--device-id=1')
        with self.assertRaises(ValueError): self.parse(r)

    def test_unstable_process(self):
        r = receipt(); r['startup']['stable_installed_process'] = False
        with self.assertRaises(ValueError): self.parse(r)

    def test_missing_real_rpc(self):
        r = receipt(); r['startup']['p4runtime']['no_pipeline_get_config'] = 'UNAVAILABLE'
        with self.assertRaises(ValueError): self.parse(r)

    def test_missing_nanomsg_client(self):
        r = receipt(); del r['startup']['nanomsg_clients']['bm_p4dbg']
        with self.assertRaises(ValueError): self.parse(r)

    def test_gdb_negative_control_false_positive(self):
        r = receipt(True); r['debug_checks'][0]['without_symbols']['found'] = True
        with self.assertRaises(ValueError): self.parse(r, True)

    def test_gdb_missing_function(self):
        r = receipt(True); r['debug_checks'][1]['with_symbols']['found'] = False
        with self.assertRaises(ValueError): self.parse(r, True)

    def test_duplicate_or_substituted_debug_target(self):
        r = receipt(True); r['debug_checks'][1] = copy.deepcopy(r['debug_checks'][0])
        with self.assertRaises(ValueError): self.parse(r, True)

    def test_missing_or_changed_pi_forwarding(self):
        for key, value in [('provider', 'usr/lib/different.so'), ('symbol', 'other'), ('source', 'other.c')]:
            r = receipt(True); r['debug_checks'][2]['debug_forwarding'][key] = value
            with self.subTest(key=key), self.assertRaises(ValueError): self.parse(r, True)
        r = receipt(True); del r['debug_checks'][2]['debug_forwarding']
        with self.assertRaises(ValueError): self.parse(r, True)

    def test_pi_lookup_keeps_source_line_and_type(self):
        for key, value in [('function', 'other'), ('source', 'other.c'), ('line', 0), ('type', '')]:
            r = receipt(True); r['debug_checks'][2]['with_symbols'][key] = value
            with self.subTest(key=key), self.assertRaises(ValueError): self.parse(r, True)
        for side, value in [('without_symbols', True), ('with_symbols', False)]:
            r = receipt(True); r['debug_checks'][2][side]['found'] = value
            with self.subTest(side=side), self.assertRaises(ValueError): self.parse(r, True)

    def test_installed_probe_reuses_declared_pi_provider(self):
        pairs = [{'path': name, 'debug_path': name + '.debug'} for name in
                 ('usr/bin/simple_switch_grpc', 'usr/lib/python3/dist-packages/_nnpy.so', 'usr/bin/pi_CLI_dummy')]
        forwarding = {'provider': 'usr/lib/x86_64-linux-gnu/libpicli.so.0', 'symbol': 'main', 'source': 'main.c',
                      'debug_path': 'usr/lib/debug/.build-id/ab/provider.debug', 'build_id': 'ab1234'}
        pairs[2]['debug_forwarding'] = forwarding
        forwarded = {'path': '/usr/bin/pi_CLI_dummy', 'debug_path': '/usr/bin/pi_CLI_dummy.debug',
                     'debug_forwarding': forwarding, 'without_symbols': {'found': False},
                     'with_symbols': {'found': True, 'function': 'main', 'source': '/build/PI/CLI/main.c',
                                      'line': 391, 'type': 'int (int, char **)'}}
        with patch.object(probe, 'installed_symbol', side_effect=lambda item, symbol: {'path': item['path'], 'symbol': symbol}) as own, \
             patch.object(probe.runtime_smoke, 'debug_pair', return_value=forwarded) as shared:
            checks = probe.installed_debug_checks({'source_debug_pairs': pairs})
            self.assertEqual(own.call_count, 2)
            shared.assert_called_once_with(pairs[2])
            self.assertEqual(checks[2]['with_symbols']['source'], 'main.c')
            self.assertEqual(checks[2]['path'], 'usr/bin/pi_CLI_dummy')
            self.assertEqual(checks[2]['debug_forwarding'], forwarding)
        for changed in [pairs[:2], pairs + [copy.deepcopy(pairs[2])],
                        pairs[:2] + [{'path': pairs[2]['path'], 'debug_path': pairs[2]['debug_path']}]]:
            with self.assertRaises(AssertionError): probe.installed_debug_checks({'source_debug_pairs': changed})
        for key in ('provider', 'symbol', 'source'):
            changed = copy.deepcopy(pairs); changed[2]['debug_forwarding'][key] = 'wrong'
            with self.subTest(key=key), self.assertRaises(AssertionError):
                probe.installed_debug_checks({'source_debug_pairs': changed})

    def test_duplicate_receipt(self):
        text = json.dumps(receipt())
        with self.assertRaises(ValueError): loaded.parse_probe(text + '\n' + text, False, 2)

    def test_rejects_host_diagnostic_injection(self):
        r = receipt(); r['environment'] = {'PATH': '/tmp/injected'}
        with self.assertRaises(ValueError): self.parse(r)

    def test_exact_artifact_hash_and_inventory(self):
        with tempfile.TemporaryDirectory() as temporary, patch.object(loaded, 'TARGETS', {'a.tar': '//:a'}):
            root = Path(temporary); path = root / 'a.tar'; path.write_bytes(b'payload')
            r = {'artifacts': {'a.tar': {'bytes': 7, 'sha256': loaded.check.sha(path)}}}
            loaded.verify_artifacts(root, r)
            path.write_bytes(b'PAYLOAD')
            with self.assertRaises(ValueError): loaded.verify_artifacts(root, r)
            r['artifacts']['../other'] = r['artifacts']['a.tar']
            with self.assertRaises(ValueError): loaded.verify_artifacts(root, r)

    def test_private_container_contract(self):
        argv = loaded.container_command('test:image', 'test-name', Path('/tmp/report'), True)
        self.assertIn('--network=none', argv); self.assertIn('--cap-drop=ALL', argv)
        self.assertIn('--cap-add=NET_ADMIN', argv); self.assertIn('--cap-add=NET_RAW', argv)
        self.assertIn('--read-only', argv); self.assertIn('--debug', argv)
        self.assertNotIn('--privileged', argv); self.assertFalse(any('/dev/net/tun' in x for x in argv))
        self.assertEqual(argv[argv.index('--entrypoint') + 1], '/usr/bin/python3')

    def test_real_symbol_positive_and_negative_controls(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary); binary = root / 'switch'; binary.write_bytes(b'ELF')
            symbols = root / 'switch.debug'; symbols.write_bytes(b'debug')
            record = {'path': str(binary), 'debug_path': str(symbols)}
            success = {'found': True, 'function': 'main', 'source': '/build/main.cpp', 'line': 17, 'type': 'int ()'}
            with patch.object(probe.runtime_smoke, 'gdb_lookup', side_effect=[{'found': False}, success]):
                self.assertEqual(probe.installed_symbol(record, 'main')['with_symbols']['source'], 'main.cpp')
            with patch.object(probe.runtime_smoke, 'gdb_lookup', return_value={'found': True}):
                with self.assertRaises(AssertionError): probe.installed_symbol(record, 'main')
            with patch.object(probe.runtime_smoke, 'gdb_lookup', side_effect=[{'found': False}, {'found': False}]):
                with self.assertRaises(AssertionError): probe.installed_symbol(record, 'main')

    def test_bmv2_cpp_main_keeps_exact_named_function_contract(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            binary = root / 'switch'; binary.write_bytes(b'ELF')
            symbols = root / 'switch.debug'; symbols.write_bytes(b'debug')
            record = {'path': 'usr/bin/simple_switch_grpc', 'debug_path': 'usr/lib/debug/switch.debug'}
            success = {'found': True, 'function': 'main(int, char**)', 'source': './main.cpp', 'line': 37,
                       'type': 'int (int, char **)', 'parameters': ['int', 'char **']}
            with patch.object(probe.runtime_smoke, 'absolute_path', side_effect=lambda name: str(binary if name == record['path'] else symbols)):
                with patch.object(probe.runtime_smoke, 'gdb_lookup', side_effect=[{'found': False}, success]):
                    self.assertEqual(probe.installed_symbol(record, 'main')['with_symbols']['function'], 'main(int, char**)')
                for function in ('main_helper', 'other::main(int, char**)', 'main(int)', 'main(int, char**) suffix'):
                    with self.subTest(function=function), patch.object(probe.runtime_smoke, 'gdb_lookup',
                            side_effect=[{'found': False}, {**success, 'function': function}]):
                        with self.assertRaises(AssertionError): probe.installed_symbol(record, 'main')
                for field, value in [('line', 0), ('source', ''), ('type', '')]:
                    with self.subTest(field=field), patch.object(probe.runtime_smoke, 'gdb_lookup',
                            side_effect=[{'found': False}, {**success, field: value}]):
                        with self.assertRaises(AssertionError): probe.installed_symbol(record, 'main')



if __name__ == '__main__':
    unittest.main()
