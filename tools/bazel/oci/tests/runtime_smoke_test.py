"""Check loader/debug probes and container isolation, without SONiC services."""
import copy
import hashlib
import json
import platform
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest
from unittest.mock import patch

from tools.bazel.ci import validate_runtime
from tools.bazel.oci import runtime_smoke


class LoaderTests(unittest.TestCase):
    def test_missing_library_and_loader_failure_are_not_success(self):
        good = {'returncode': 0, 'stdout': 'libx.so => /usr/lib/libx.so (0x123)', 'stderr': ''}
        self.assertTrue(runtime_smoke.loader_result(good, ['libx.so']))
        self.assertFalse(runtime_smoke.loader_result({**good, 'stdout': 'libx.so => not found'}, ['libx.so']))
        self.assertFalse(runtime_smoke.loader_result({**good, 'returncode': 127}, ['libx.so']))
        self.assertFalse(runtime_smoke.loader_result({**good, 'returncode': None}, ['libx.so']))

    def test_static_exception_requires_no_declared_dynamic_dependencies(self):
        static = {'returncode': 1, 'stdout': '', 'stderr': 'not a dynamic executable'}
        self.assertTrue(runtime_smoke.loader_result(static, []))
        self.assertFalse(runtime_smoke.loader_result(static, ['libc.so.6']))

    def test_path_traversal_is_rejected(self):
        for path in ['../etc/passwd', '/usr/../../etc/passwd', 'usr/bin/x\x00']:
            with self.subTest(path=path), self.assertRaises(ValueError):
                runtime_smoke.absolute_path(path)

    def test_no_architecture_emulation_is_misreported_as_native(self):
        with patch.object(runtime_smoke.platform, 'machine', return_value='aarch64'):
            with self.assertRaisesRegex(ValueError, 'Native machine'):
                runtime_smoke.probe({'architecture': 'amd64'}, 'runtime')

    def test_missing_installed_supervisor_command_fails(self):
        with tempfile.TemporaryDirectory() as temp:
            config = Path(temp) / 'supervisord.conf'
            config.write_text('[program:service]\ncommand=/nonexistent-service\n')
            with self.assertRaisesRegex(ValueError, 'unavailable'):
                runtime_smoke.service_checks({'supervisor_config': str(config)})

    def test_startup_profile_executes_the_script_and_checks_its_bytes(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            source, target = root/'input', root/'output'
            script = root/'start.sh'
            script.write_text('#!/bin/sh\ncp "' + str(source) + '" "' + str(target) + '"\n')
            script.chmod(0o755)
            policy = {'startup_profile': {'script': str(script), 'input': str(source),
                                          'output': str(target), 'contents': 'SAI_TEST=value\n'}}
            self.assertTrue(runtime_smoke.service_checks(policy)['startup_profile']['matched'])
            script.write_text('#!/bin/sh\nprintf changed > "' + str(target) + '"\n')
            with self.assertRaisesRegex(ValueError, 'expected SAI profile'):
                runtime_smoke.service_checks(policy)


class P4CompilerTests(unittest.TestCase):
    def test_driver_directory_and_direct_backend_file_outputs(self):
        for directory_output in (True, False):
            with self.subTest(directory_output=directory_output):
                config = {'compiler': '/usr/bin/p4c-bm2-ss', 'switch': '/usr/bin/simple_switch_grpc'}
                if directory_output:
                    config.update(compiler='/usr/bin/p4c', compiler_args=['--target', 'bmv2'],
                                  compiler_output_mode='directory')

                def compile_program(argv, timeout):
                    # Model the external CLI contracts: p4c writes <source>.json
                    # inside -o; its direct BMv2 backend writes the -o file.
                    source = next(Path(arg) for arg in argv if arg.endswith('.p4'))
                    output = Path(argv[argv.index('-o') + 1])
                    if directory_output:
                        self.assertEqual(argv[1:3], ['--target', 'bmv2'])
                        output.mkdir(exist_ok=True)
                        output = output / (source.stem + '.json')
                    output.write_text(json.dumps({'pipelines': [{'name': 'ingress'}]}))
                    return {'returncode': 0, 'stdout': '', 'stderr': ''}

                with patch.object(runtime_smoke, 'command', side_effect=compile_program), \
                     patch.object(runtime_smoke.subprocess, 'Popen') as launch, \
                     patch.object(runtime_smoke.socket, 'create_connection'):
                    launch.return_value.poll.return_value = None
                    result = runtime_smoke.p4_service_check(config)
                self.assertTrue(result['grpc_listener_ready'])
                self.assertEqual(result['compiler_output_mode'], 'directory' if directory_output else 'file')
                launch.return_value.terminate.assert_called_once()


class RedisFixtureTests(unittest.TestCase):
    def test_wrong_helper_bytes_fail_before_starting_a_process(self):
        with tempfile.TemporaryDirectory() as temp:
            helper = Path(temp) / 'redis'
            helper.write_bytes(b'changed')
            with patch.object(runtime_smoke.subprocess, 'Popen') as launch:
                with self.assertRaisesRegex(ValueError, 'recorded bytes'):
                    with runtime_smoke.redis_fixture({'executable': str(helper), 'sha256': 'wrong'}):
                        self.fail('Unexpected fixture admission')
            launch.assert_not_called()

    def test_unexpected_database_socket_fails_before_starting_a_process(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp); helper = root / 'redis'; helper.write_bytes(b'fixture')
            config = root / 'database.json'
            config.write_text(json.dumps({'INSTANCES': {'redis': {'unix_socket_path': '/tmp/other.sock'}}}))
            params = {'executable': str(helper), 'sha256': hashlib.sha256(helper.read_bytes()).hexdigest(),
                      'database_config': str(config)}
            with patch.object(runtime_smoke, 'command', return_value={'returncode': 0, 'stdout': 'Redis server v=7'}):
                with patch.object(runtime_smoke.subprocess, 'Popen') as launch:
                    with self.assertRaisesRegex(ValueError, 'Unexpected Redis socket'):
                        with runtime_smoke.redis_fixture(params):
                            self.fail('Unexpected fixture admission')
                launch.assert_not_called()

    def test_redis_mounts_are_a_readonly_helper_and_a_fresh_anonymous_volume(self):
        args = validate_runtime.container_arguments('id', 'name', Path(__file__), [], Path('/usr/bin/redis-server'))
        self.assertIn('type=volume,dst=/run/redis', args)
        self.assertTrue(any('dst=/probe/redis-server,readonly' in value for value in args))
        self.assertFalse(any('src=/run/redis' in value or 'src=/var/run/redis' in value for value in args))
        self.assertIn('--rm', args)  # Docker removes this container's anonymous volume.


class DockerIdentityTests(unittest.TestCase):
    def setUp(self):
        self.digest = 'sha256:' + 'a'*64
        self.config = {'architecture': 'amd64', 'os': 'linux',
                       'config': {'Env': ['PATH=/usr/bin'], 'Entrypoint': ['/usr/bin/start']},
                       'rootfs': {'type': 'layers', 'diff_ids': ['sha256:'+'b'*64]}}
        self.image = {'Id': self.digest, 'Architecture': 'amd64', 'Os': 'linux',
                      'Config': copy.deepcopy(self.config['config']),
                      'RootFS': {'Type': 'layers', 'Layers': list(self.config['rootfs']['diff_ids'])}}

    def test_classic_and_containerd_identities_verify_actual_configuration_and_layers(self):
        validate_runtime.verify_loaded_image(self.image, self.config, self.digest)
        self.image['Id'] = 'sha256:'+'c'*64
        self.image['Descriptor'] = {'digest': self.image['Id'],
                                    'mediaType': 'application/vnd.docker.distribution.manifest.v2+json'}
        validate_runtime.verify_loaded_image(self.image, self.config, self.digest)
        for field, value in [('Config', {'Env': ['PATH=/changed']}), ('RootFS', {'Type':'layers','Layers':[]})]:
            wrong = {**self.image, field: value}
            with self.subTest(field=field), self.assertRaises(ValueError):
                validate_runtime.verify_loaded_image(wrong, self.config, self.digest)

    def test_unexplained_image_identity_change_fails(self):
        with self.assertRaisesRegex(ValueError, 'identity'):
            validate_runtime.verify_loaded_image({**self.image, 'Id':'sha256:'+'c'*64}, self.config, self.digest)

    def test_container_has_no_network_capabilities_or_host_service_mounts(self):
        args = validate_runtime.container_arguments('sha256:123', 'unique-name', Path(__file__), ['/etc/sai.d'])
        for flag in ['--network=none', '--cap-drop=ALL', '--security-opt=no-new-privileges', '--read-only', '--rm']:
            self.assertIn(flag, args)
        self.assertNotIn('--privileged', args)
        self.assertNotIn('/var/run/docker.sock', ' '.join(args))
        self.assertIn('readonly', args[args.index('--mount')+1])
        for bad in ['/', '/proc', '/sys/x', '/dev', '/tmp/../etc']:
            with self.subTest(path=bad), self.assertRaises(ValueError):
                validate_runtime.container_arguments('id', 'name', Path(__file__), [bad])

    def test_wrong_archive_bytes_fail_before_docker(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp); archive=root/'image.gz'; archive.write_bytes(b'wrong')
            report={'native':{'paired_symbols':[{'path':'usr/bin/app','debug_path':'usr/lib/debug/app'}],
                              'embedded_symbols':[],'preserved_make_debug_gaps':[],
                              'inherited_base_symbol_pairs':[]},
                    'images':{'archives':{'runtime':{'sha256':hashlib.sha256(b'expected').hexdigest(),'bytes':8}}}}
            (root/'report.json').write_text(json.dumps(report));(root/'policy.json').write_text(json.dumps({'architecture': {'x86_64':'amd64','aarch64':'arm64'}[platform.machine()]}))
            with patch.object(validate_runtime, 'load_archive') as load:
                with self.assertRaisesRegex(ValueError, 'differs from the static'):
                    validate_runtime.validate(archive,archive,root/'report.json',root/'policy.json',root/'out.json')
            load.assert_not_called()


@unittest.skipUnless(all(shutil.which(x) for x in ['cc','objcopy','readelf','gdb']), 'native C/binutils/GDB required')
class NativeDebugTests(unittest.TestCase):
    def test_forwarded_main_resolves_its_exact_installed_source(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            provider, launcher = root / 'libforward.so.0', root / 'launcher'
            source = root / 'implementation.c'
            source.write_text('int unrelated(void) { return 42; }\nint main(void) { return 0; }\n')
            subprocess.run(['cc', '-shared', '-fPIC', '-g', '-Wl,--build-id=sha1',
                            '-Wl,-soname,libforward.so.0', str(source), '-o', str(provider)],
                           check=True, capture_output=True)
            subprocess.run(['cc', '-Wl,--build-id=sha1', '-Wl,--no-as-needed', str(provider), '-o', str(launcher)],
                           check=True, capture_output=True)
            debug = root / 'symbols'
            symbol_files = {}
            for binary in (provider, launcher):
                notes = subprocess.check_output(['readelf', '-n', str(binary)], text=True)
                identifier = notes.split('Build ID: ', 1)[1].split()[0]
                symbols = debug / '.build-id' / identifier[:2] / (identifier[2:] + '.debug')
                symbols.parent.mkdir(parents=True)
                subprocess.run(['objcopy', '--only-keep-debug', str(binary), str(symbols)], check=True, capture_output=True)
                subprocess.run(['objcopy', '--strip-debug', '--add-gnu-debuglink=' + str(symbols), str(binary)],
                               check=True, capture_output=True)
                symbol_files[binary] = symbols
            record = {'path': str(launcher), 'debug_path': str(symbol_files[launcher]),
                      'debug_forwarding': {'provider': str(provider), 'debug_path': str(symbol_files[provider]),
                                           'symbol': 'main', 'source': 'implementation.c'}}
            actual_lookup = runtime_smoke.gdb_lookup
            def lookup(binary, directory, debug_directory, symbol=None):
                return actual_lookup(binary, directory, str(debug) if debug_directory == '/usr/lib/debug'
                                     else debug_directory, symbol)
            with patch.object(runtime_smoke, 'gdb_lookup', side_effect=lookup):
                result = runtime_smoke.debug_pair(record)
                self.assertFalse(result['without_symbols']['found'])
                self.assertEqual(result['with_symbols']['function'], 'main')
                self.assertEqual(result['with_symbols']['line'], 2)
                record['debug_forwarding']['source'] = 'wrong.c'
                with self.assertRaisesRegex(ValueError, 'declared source'):
                    runtime_smoke.debug_pair(record)
                record['debug_forwarding']['source'] = 'implementation.c'
                record['debug_forwarding']['symbol'] = 'absent_function'
                with self.assertRaisesRegex(ValueError, 'do not provide source'):
                    runtime_smoke.debug_pair(record)

    def test_installed_symbol_lookup_fails_without_the_matching_debug_file(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp); binary=root/'libanswer.so'; debug=root/'symbols'; empty=root/'empty'
            debug.mkdir();empty.mkdir();source=root/'answer.c';source.write_text('int answer(int value) { return value + 42; }\n')
            subprocess.run(['cc','-shared','-fPIC','-g','-Wl,--build-id','-o',str(binary),str(source)],check=True,capture_output=True)
            notes=subprocess.check_output(['readelf','-n',str(binary)],text=True)
            identifier=notes.split('Build ID: ',1)[1].split()[0]
            symbols=debug/'.build-id'/identifier[:2]/(identifier[2:]+'.debug');symbols.parent.mkdir(parents=True)
            subprocess.run(['objcopy','--only-keep-debug',str(binary),str(symbols)],check=True,capture_output=True)
            subprocess.run(['objcopy','--strip-debug',str(binary)],check=True,capture_output=True)
            self.assertFalse(runtime_smoke.gdb_lookup(binary,root,str(empty))['found'])
            lookup=runtime_smoke.gdb_lookup(binary,root,str(debug))
            self.assertTrue(lookup['found'],lookup)
            self.assertEqual(lookup['line'],1)
            self.assertIn('answer.c',lookup['source'])
            self.assertTrue(lookup['parameters'])
            # A symbol file from a different build must not satisfy the lookup.
            source.write_text('int different(int value) { return value - 3; }\n')
            other = root / 'other.so'
            subprocess.run(['cc','-shared','-fPIC','-g','-Wl,--build-id','-o',str(other),str(source)],check=True,capture_output=True)
            subprocess.run(['objcopy','--only-keep-debug',str(other),str(symbols)],check=True,capture_output=True)
            self.assertFalse(runtime_smoke.gdb_lookup(binary,root,str(debug))['found'])
            self.assertTrue(runtime_smoke.loader_result(runtime_smoke.command(['ldd',str(binary)]),[]))


if __name__=='__main__':
    unittest.main()
