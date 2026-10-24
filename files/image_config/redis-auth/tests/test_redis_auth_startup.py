"""Selected-process startup and real Redis read/write checks. Run as root."""

import copy
import importlib.util
import json
import os
from pathlib import Path
import pwd
import socket
import stat
import subprocess
import sys
import tempfile
import time
import unittest
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import redis_auth_startup as startup

ROOT = Path(__file__).resolve().parents[4]
spec = importlib.util.spec_from_file_location('redis_auth_configure', ROOT / 'dockers/docker-database/redis_auth_configure.py')
configure = importlib.util.module_from_spec(spec)
spec.loader.exec_module(configure)
SERVER = ROOT / 'dockers/docker-database/redis_auth_server.py'
spec = importlib.util.spec_from_file_location('redis_auth_server', SERVER)
server = importlib.util.module_from_spec(spec)
spec.loader.exec_module(server)


class Redis:
    def __init__(self, path):
        self.sock = socket.socket(socket.AF_UNIX)
        self.sock.settimeout(3)
        self.sock.connect(str(path))
        self.stream = self.sock.makefile('rb')

    def close(self):
        self.stream.close()
        self.sock.close()

    def command(self, *arguments):
        words = [str(word).encode() if not isinstance(word, bytes) else word for word in arguments]
        self.sock.sendall(b'*%d\r\n' % len(words) + b''.join(b'$%d\r\n' % len(word) + word + b'\r\n' for word in words))
        return self.reply()

    def reply(self):
        marker = self.stream.read(1)
        line = self.stream.readline().rstrip(b'\r\n')
        if marker == b'-':
            raise RuntimeError(line.split(b' ', 1)[0].decode())
        if marker == b'$':
            length = int(line)
            if length == -1:
                return None
            data = self.stream.read(length)
            self.stream.read(2)
            return data
        if marker == b'*':
            return [self.reply() for _ in range(int(line))]
        if marker == b':':
            return int(line)
        if marker == b'+':
            return line
        raise RuntimeError('invalid Redis response')


class StartupTest(unittest.TestCase):
    def setUp(self):
        if os.geteuid() != 0:
            self.skipTest('requires root in an isolated container')
        temporary = tempfile.TemporaryDirectory(prefix='redis-startup-')
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        self.root.chmod(0o755)
        self.runtime = self.root / 'run'
        self.intent = self.root / 'intent.json'
        enabled = mock.patch.object(startup, 'build_enabled', return_value=True)
        enabled.start()
        self.addCleanup(enabled.stop)
        groups = mock.patch.object(startup, 'socket_groups', return_value=[100, 1001])
        groups.start()
        self.addCleanup(groups.stop)

    def select(self, **modes):
        if set(modes).issubset({'redis', 'redis_bmp'}):
            enforce = any(mode == 'enforce' for mode in modes.values())
            modes = {'mode': 'enforce' if enforce else 'legacy',
                     'tcp_exceptions': [name for name in ('redis', 'redis_bmp')
                                        if enforce and modes.get(name) != 'enforce']}
        self.intent.write_text(json.dumps(modes))
        self.intent.chmod(0o644)

    def prepare(self):
        return startup.prepare_selected(intent_path=self.intent, runtime=self.runtime)

    def test_absent_or_legacy_is_inactive(self):
        self.assertEqual(self.prepare(), [])
        self.select(redis='legacy', redis_bmp='legacy')
        self.assertEqual(self.prepare(), [])
        self.assertFalse(self.runtime.exists())

    def test_explicit_primary_and_bmp_share_one_boot_pair(self):
        self.select(redis='enforce', redis_bmp='enforce')
        self.assertEqual(self.prepare(), ['redis', 'redis_bmp'])
        policy = json.loads((self.runtime / 'local.json').read_text())
        self.assertEqual({entry['unix_socket'] for entry in policy}, {item['unix_socket_path'] for item in startup.configured_instances().values()})
        self.assertTrue(all(entry['tcp_port'] == 0 for entry in policy))
        secret = self.runtime / 'local-system/writer.secret'
        inode = secret.stat().st_ino
        self.prepare()
        self.assertEqual(secret.stat().st_ino, inode)
        self.assertEqual(stat.S_IMODE(secret.stat().st_mode), 0o400)
        self.assertEqual(stat.S_IMODE((self.runtime / 'local.json').stat().st_mode), 0o644)
        self.assertEqual(stat.S_IMODE((self.runtime / 'server/users.acl').stat().st_mode), 0o644)
        self.assertEqual(json.loads((self.runtime / 'server/socket-groups.json').read_text()), [100, 1001])

    def test_systemd_umask_prepares_both_processes(self):
        self.select(redis='enforce', redis_bmp='enforce')
        previous = os.umask(0o077)
        try:
            self.assertEqual(self.prepare(), ['redis', 'redis_bmp'])
            inode = (self.runtime / 'local-system/writer.secret').stat().st_ino
            self.prepare()
            self.assertTrue(startup.runtime_required(intent_path=self.intent, runtime=self.runtime))
        finally:
            os.umask(previous)
        self.assertEqual(stat.S_IMODE((self.runtime / 'server').stat().st_mode), 0o755)
        for name in ('local.json', 'server/users.acl'):
            self.assertEqual(stat.S_IMODE((self.runtime / name).stat().st_mode), 0o644)
        for name in ('writer.secret', 'writer.sha256'):
            self.assertEqual(stat.S_IMODE((self.runtime / 'local-system' / name).stat().st_mode), 0o400)
        self.assertEqual((self.runtime / 'local-system/writer.secret').stat().st_ino, inode)

    def test_unknown_process_or_mode_rejected(self):
        for content in ({'remote_redis': 'enforce'}, {'redis': 'oops'}, ['redis']):
            self.intent.write_text(json.dumps(content))
            self.intent.chmod(0o644)
            with self.assertRaises(startup.CredentialStateError):
                self.prepare()
        self.assertFalse(self.runtime.exists())

    def test_world_writable_intent_rejected(self):
        self.select(redis='enforce')
        self.intent.chmod(0o666)
        with self.assertRaises(startup.CredentialStateError):
            self.prepare()

    def test_symlink_intent_rejected(self):
        self.select(redis='enforce')
        link = self.root / 'link'
        link.symlink_to(self.intent)
        with self.assertRaises(OSError):
            startup.prepare_selected(intent_path=link, runtime=self.runtime)

    def test_same_boot_selection_cannot_change(self):
        self.select(redis='enforce')
        self.prepare()
        self.select(redis='enforce', redis_bmp='enforce')
        with self.assertRaises(startup.CredentialStateError):
            self.prepare()
        self.select(redis='legacy')
        with self.assertRaises(startup.CredentialStateError):
            self.prepare()

    def test_startup_check_fails_without_prepared_runtime(self):
        self.select(redis='enforce')
        with self.assertRaises(OSError):
            startup.runtime_required(intent_path=self.intent, runtime=self.runtime)
        self.assertFalse(self.runtime.exists())

    def test_startup_check_rejects_missing_policy_or_acl(self):
        self.select(redis='enforce')
        self.prepare()
        self.assertTrue(startup.runtime_required(intent_path=self.intent, runtime=self.runtime))
        (self.runtime / 'local.json').unlink()
        with self.assertRaises(OSError):
            startup.runtime_required(intent_path=self.intent, runtime=self.runtime)
        self.assertFalse((self.runtime / 'local.json').exists())

    def test_startup_check_rejects_changed_acl(self):
        self.select(redis='enforce')
        self.prepare()
        (self.runtime / 'server/users.acl').write_text('user default on nopass +@all')
        with self.assertRaises(startup.CredentialStateError):
            startup.runtime_required(intent_path=self.intent, runtime=self.runtime)

    def test_startup_check_rejects_missing_or_changed_socket_groups(self):
        self.select(redis='enforce')
        self.prepare()
        groups = self.runtime / 'server/socket-groups.json'
        groups.write_text('[100, 1234]')
        with self.assertRaises(startup.CredentialStateError):
            startup.runtime_required(intent_path=self.intent, runtime=self.runtime)
        groups.unlink()
        with self.assertRaises(OSError):
            startup.runtime_required(intent_path=self.intent, runtime=self.runtime)

    def test_partial_credential_is_not_replaced(self):
        self.select(redis='enforce')
        self.prepare()
        (self.runtime / 'local-system/writer.secret').unlink()
        with self.assertRaises(startup.CredentialStateError):
            self.prepare()

    def test_unsafe_server_directory_rejected(self):
        self.select(redis='enforce')
        self.prepare()
        for mode in (0o700, 0o750, 0o777):
            with self.subTest(mode=oct(mode)):
                (self.runtime / 'server').chmod(mode)
                with self.assertRaises(startup.CredentialStateError):
                    self.prepare()
                self.assertEqual(stat.S_IMODE((self.runtime / 'server').stat().st_mode), mode)

    def database(self):
        return {'INSTANCES': startup.configured_instances()}

    def additional(self):
        return {'INSTANCES': {name: {'is_protected_mode': True} for name in startup.configured_instances()}}

    def configured(self, database=None, additional=None, policy=None, environ=None, chassis=False):
        if policy is None:
            self.select(redis='enforce', redis_bmp='enforce')
            self.prepare()
            policy = json.loads((self.runtime / 'local.json').read_text())
        return configure.configure(database or self.database(), additional or self.additional(), policy,
                                   json.loads((self.runtime / 'server/instances.json').read_text()),
                                   environ=environ or {}, chassis=chassis)

    def test_only_selected_process_metadata_changes(self):
        self.select(redis='enforce')
        self.prepare()
        database, additional = self.database(), self.additional()
        bmp = copy.deepcopy(database['INSTANCES']['redis_bmp'])
        selected = self.configured(database, additional, json.loads((self.runtime / 'local.json').read_text()))
        self.assertEqual(selected, {'redis'})
        self.assertEqual(database['INSTANCES']['redis']['port'], 0)
        self.assertTrue(additional['INSTANCES']['redis']['redis_auth_enforce'])
        self.assertEqual(database['INSTANCES']['redis_bmp'], bmp)

    def test_known_remote_and_namespace_topologies_rejected(self):
        for environ in ({'NAMESPACE_ID': '0'}, {'NAMESPACE_COUNT': '2'}, {'DATABASE_TYPE': 'dpudb'},
                        {'DATABASE_TYPE': 'chassisdb'}, {'NUM_DPU': '1'}):
            with self.assertRaises(ValueError):
                self.configured(environ=environ)
        with self.assertRaises(ValueError):
            self.configured(chassis=True)

    def test_actual_endpoint_must_match(self):
        for field, value in (('hostname', '10.0.0.1'), ('unix_socket_path', '/tmp/other.sock'),
                             ('database_type', 'chassisdb')):
            database = self.database()
            database['INSTANCES']['redis'][field] = value
            with self.assertRaises(ValueError):
                self.configured(database=database)
        additional = self.additional()
        additional['INSTANCES']['redis']['bmc_link_ip'] = '10.0.0.2'
        with self.assertRaises(ValueError):
            self.configured(additional=additional)

    def test_ambiguous_or_tcp_policy_rejected(self):
        self.select(redis='enforce')
        self.prepare()
        entry = json.loads((self.runtime / 'local.json').read_text())[0]
        with self.assertRaises(ValueError):
            self.configured(policy=[entry, entry])
        entry['tcp_port'] = 6379
        with self.assertRaises(ValueError):
            self.configured(policy=entry)

    def test_missing_selected_process_and_changed_exception_are_rejected(self):
        self.select(mode='enforce')
        self.prepare()
        policy = json.loads((self.runtime / 'local.json').read_text())
        with self.assertRaises(ValueError):
            self.configured(policy=policy[:1])
        self.select(mode='legacy')
        # Use a separate boot directory for the deliberately mixed selection.
        self.runtime = self.root / 'mixed-run'
        self.select(mode='enforce', tcp_exceptions=['redis_bmp'])
        self.prepare()
        database = self.database()
        database['INSTANCES']['redis_bmp']['port'] += 1
        with self.assertRaises(ValueError):
            self.configured(database=database, policy=json.loads((self.runtime / 'local.json').read_text()))

    def test_redis_uid_reads_acl_but_not_password(self):
        self.select(redis='enforce')
        self.prepare()
        identity = pwd.getpwnam('redis')
        secret = self.runtime / 'local-system/writer.secret'
        acl = self.runtime / 'server/users.acl'
        script = 'from pathlib import Path; import sys; Path(sys.argv[1]).read_bytes()'
        def demote():
            os.setgroups([])
            os.setgid(identity.pw_gid)
            os.setuid(identity.pw_uid)
        self.assertEqual(subprocess.run([sys.executable, '-c', script, str(acl)], preexec_fn=demote,
                                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL).returncode, 0)
        self.assertNotEqual(subprocess.run([sys.executable, '-c', script, str(secret)], preexec_fn=demote,
                                          stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL).returncode, 0)

    def test_supervisor_template_keeps_legacy_and_enforces_selected(self):
        import jinja2
        template = jinja2.Template((ROOT / 'dockers/docker-database/supervisord.conf.j2').read_text())
        database, additional = self.database(), self.additional()
        self.select(redis='enforce')
        self.prepare()
        self.configured(database, additional, json.loads((self.runtime / 'local.json').read_text()))
        for name, extra in additional['INSTANCES'].items():
            database['INSTANCES'][name].update(extra)
        rendered = template.render(**database)
        commands = [line for line in rendered.splitlines() if 'exec ' in line and '/usr/bin/redis-server' in line]
        self.assertEqual(len(commands), 2)
        selected = next(line for line in commands if '--unixsocket /var/run/redis/redis.sock ' in line)
        legacy = next(line for line in commands if '--unixsocket /var/run/redis/redis_bmp.sock ' in line)
        self.assertIn('--port 0 ', selected)
        self.assertIn('exec /usr/local/bin/redis_auth_server.py /var/run/redis/redis.sock /usr/bin/redis-server', selected)
        self.assertIn('--unixsocketperm 760 --aclfile /run/redis-auth/server/users.acl', selected)
        self.assertIn('stopasgroup=true', rendered)
        self.assertIn('killasgroup=true', rendered)
        self.assertNotIn('redis_auth_server.py', legacy)
        self.assertNotIn('--aclfile', legacy)

    def test_container_control_template_is_valid_bash(self):
        import jinja2
        environment = jinja2.Environment(undefined=jinja2.ChainableUndefined)
        template = environment.from_string((ROOT / 'files/build_templates/docker_image_ctl.j2').read_text())
        for name in ('database', 'swss', 'pmon', 'bgp', 'snmp'):
            rendered = template.render(docker_container_name=name, sonic_asic_platform='vs',
                                       docker_image_run_opt='', docker_image_name='test',
                                       install_debug_image='n', mount_default_tmpfs='n')
            result = subprocess.run(['bash', '-n'], input=rendered, text=True, capture_output=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertIn('REDIS_AUTH_REQUIRED=1', rendered)
            if name == 'database':
                self.assertNotIn('redis_socket_mode', rendered)
                for socket in ('REDIS_BMP_SOCK', 'REDIS_SOCK'):
                    self.assertIn(f'chgrp -f redis ${socket} && chmod -f 0760 ${socket}', rendered)
            self.assertIn('redis-auth-delivery-missing', rendered)
            self.assertIn('sudo -n /usr/bin/redis_auth_startup.py --check', rendered)
            self.assertIn('SONIC_DB_CLI=\"sudo -n $SONIC_DB_CLI\"', rendered)
            self.assertIn('SONIC_CFGGEN=\"sudo -n $SONIC_CFGGEN\"', rendered)
            self.assertIn('\"${REDIS_AUTH_MNT[@]}\"', rendered)
            self.assertNotIn('redis_socket_mode', rendered)
            if name == 'swss':
                self.assertLess(rendered.index('SONIC_DB_CLI=\"sudo -n $SONIC_DB_CLI\"'),
                                rendered.index("$SONIC_DB_CLI CONFIG_DB HDEL 'DEVICE_METADATA|localhost'"))
                self.assertLess(rendered.index('SONIC_CFGGEN=\"sudo -n $SONIC_CFGGEN\"'),
                                rendered.index('$SONIC_CFGGEN -j \"$RUNTIME_VARS_JSON\" --write-to-db'))

    def test_container_reuse_matches_current_boot_mode(self):
        import jinja2
        environment = jinja2.Environment(undefined=jinja2.ChainableUndefined)
        template = environment.from_string((ROOT / 'files/build_templates/docker_image_ctl.j2').read_text())
        for name in ('database', 'swss', 'pmon', 'bgp', 'snmp'):
            rendered = template.render(docker_container_name=name, sonic_asic_platform='vs',
                                       docker_image_run_opt='', docker_image_name='test',
                                       install_debug_image='n', mount_default_tmpfs='n')
            start = rendered.index('        if [ "$REDIS_AUTH_MODE" == "enforce" ] && ! jq -e')
            end = rendered.index('        if [ x"$DOCKERMOUNT" == x"$MOUNTPATH" ]; then', start)
            validation = rendered[start:end]
            mount = {'Source': '/run/redis-auth', 'Destination': '/run/redis-auth', 'RW': False}
            for mode, mounts, marker, expected in (
                    ('legacy', [], [], ''),
                    ('legacy', [mount], [], 'redis-auth-delivery-obsolete'),
                    ('legacy', [], ['REDIS_AUTH_REQUIRED=1'], 'redis-auth-delivery-obsolete'),
                    ('legacy', [mount], ['REDIS_AUTH_REQUIRED=1'], 'redis-auth-delivery-obsolete'),
                    ('enforce', [], [], 'redis-auth-delivery-missing'),
                    ('enforce', [mount], [], 'redis-auth-delivery-missing'),
                    ('enforce', [mount], ['REDIS_AUTH_REQUIRED=1'], '')):
                inspected = json.dumps([{'Mounts': mounts, 'Config': {'Env': marker}}])
                shell = ('REDIS_AUTH_MODE=$1; DOCKERCHECK=$2; DOCKERMOUNT=""\n'
                         + validation + '\nprintf "%s" "$DOCKERMOUNT"\n')
                result = subprocess.run(['bash', '-c', shell, '--', mode, inspected],
                                        text=True, capture_output=True)
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertEqual(result.stdout, expected, (name, mode, mounts, marker))

    def test_real_redis_readers_and_writers_on_all_databases(self):
        self.select(redis='enforce', redis_bmp='enforce')
        previous = os.umask(0o077)
        try:
            self.prepare()
        finally:
            os.umask(previous)
        identity = pwd.getpwnam('redis')
        socket_dir = self.root / 'sockets'
        socket_dir.mkdir()
        os.chown(socket_dir, identity.pw_uid, identity.pw_gid)
        def demote():
            os.setgroups([])
            os.setgid(identity.pw_gid)
            os.setuid(identity.pw_uid)
        for name in startup.configured_instances():
            path = socket_dir / (name + '.sock')
            process = subprocess.Popen([sys.executable, str(SERVER), '--groups',
                                        str(self.runtime / 'server/socket-groups.json'), str(path),
                                        '/usr/bin/redis-server', '--port', '0', '--unixsocket', str(path),
                                        '--unixsocketperm', '760', '--save', '', '--appendonly', 'no',
                                        '--databases', '100', '--aclfile', str(self.runtime / 'server/users.acl')],
                                       preexec_fn=demote, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            self.addCleanup(lambda process=process: process.wait(timeout=5) if process.poll() is not None
                            else (process.terminate(), process.wait(timeout=5)))
            for _ in range(100):
                if path.exists():
                    break
                if process.poll() is not None:
                    self.fail('Redis rejected generated policy')
                time.sleep(0.01)
            reader, writer = Redis(path), Redis(path)
            self.addCleanup(reader.close)
            self.addCleanup(writer.close)
            password = (self.runtime / 'local-system/writer.secret').read_bytes()
            self.assertEqual(writer.command('AUTH', startup.USERNAME, password), b'OK')
            for database in range(100):
                self.assertEqual(writer.command('SELECT', database), b'OK')
                self.assertEqual(reader.command('SELECT', database), b'OK')
                self.assertEqual(writer.command('SET', 'test:key', 'before'), b'OK')
                self.assertEqual(reader.command('GET', 'test:key'), b'before')
                with self.assertRaisesRegex(RuntimeError, 'NOPERM'):
                    reader.command('SET', 'test:key', 'bad')
            self.assertEqual(reader.command('EVAL', "return redis.call('GET', KEYS[1])", 1, 'test:key'), b'before')
            with self.assertRaisesRegex(RuntimeError, 'ERR|NOPERM'):
                reader.command('EVAL', "return redis.call('SET', KEYS[1], 'bad')", 1, 'test:key')
            self.assertEqual(reader.command('GET', 'test:key'), b'before')
            # Optional library metadata is unavailable on 7.0 and denied to readers on newer Redis.
            with self.assertRaisesRegex(RuntimeError, 'ERR|NOPERM'):
                reader.command('CLIENT', 'SETINFO', 'LIB-NAME', 'ordinary-reader')
            self.assertEqual(reader.command('PING'), b'PONG')
            for command in (('CLIENT', 'KILL', 'TYPE', 'normal'), ('FLUSHDB',),
                            ('PUBLISH', 'test:channel', 'bad'),
                            ('CONFIG', 'SET', 'timeout', 0), ('ACL', 'SETUSER', 'test-invalid')):
                with self.assertRaisesRegex(RuntimeError, 'NOPERM'):
                    reader.command(*command)
            self.assertEqual(reader.command('MULTI'), b'OK')
            with self.assertRaisesRegex(RuntimeError, 'NOPERM'):
                reader.command('SET', 'test:key', 'bad')
            with self.assertRaisesRegex(RuntimeError, 'EXECABORT'):
                reader.command('EXEC')
            self.assertEqual(reader.command('GET', 'test:key'), b'before')
            self.wait_for_reader_acl(path, process)
            self.assertEqual(stat.S_IMODE(path.stat().st_mode), 0o760)
            # Named writers retain existing administrative commands.
            self.assertEqual(writer.command('CONFIG', 'SET', 'timeout', 0), b'OK')

    def wait_for_reader_acl(self, path, process):
        for _ in range(200):
            self.assertIsNone(process.poll(), 'Redis or its socket helper exited')
            if path.exists():
                acl = subprocess.check_output(['getfacl', '-ncp', str(path)], text=True)
                if 'group:100:rw-' in acl:
                    return
            time.sleep(0.02)
        self.fail('selected socket never granted reader access')

    def start_socket_server(self, path, *, enforce):
        identity = pwd.getpwnam('redis')
        path.parent.mkdir(exist_ok=True)
        path.parent.chmod(0o755)
        os.chown(path.parent, identity.pw_uid, identity.pw_gid)
        command = ['/usr/bin/redis-server', '--port', '0', '--unixsocket', str(path),
                   '--unixsocketperm', '760', '--save', '', '--appendonly', 'no',
                   '--pidfile', str(path.with_suffix('.pid'))]
        if enforce:
            command += ['--aclfile', str(self.runtime / 'server/users.acl')]
            command = [sys.executable, str(SERVER), '--groups',
                       str(self.runtime / 'server/socket-groups.json'), str(path)] + command

        def demote():
            os.setgroups([])
            os.setgid(identity.pw_gid)
            os.setuid(identity.pw_uid)

        process = subprocess.Popen(command, preexec_fn=demote,
                                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        self.addCleanup(lambda: process.wait(timeout=5) if process.poll() is not None
                        else (process.terminate(), process.wait(timeout=5)))
        for _ in range(200):
            self.assertIsNone(process.poll(), 'Redis or its socket helper exited')
            if path.exists():
                break
            time.sleep(0.02)
        if enforce:
            self.wait_for_reader_acl(path, process)
        return process

    def as_reader(self, path, *command, gid=100):
        def demote():
            os.setgroups([])
            os.setgid(gid)
            os.setuid(65534)

        return subprocess.run(['redis-cli', '-s', str(path), '--raw', '-e', *command],
                              preexec_fn=demote, text=True, capture_output=True)

    def test_mixed_sockets_admit_existing_readers_without_opening_legacy(self):
        self.select(redis='enforce')
        self.prepare()
        selected, legacy = self.root / 'sockets/redis.sock', self.root / 'sockets/redis_bmp.sock'
        self.start_socket_server(selected, enforce=True)
        self.start_socket_server(legacy, enforce=False)
        for gid in (100, 1001, pwd.getpwnam('redis').pw_gid):
            result = self.as_reader(selected, 'PING', gid=gid)
            self.assertEqual((result.returncode, result.stdout.strip()), (0, 'PONG'))
        denied = self.as_reader(selected, 'SET', 'test:reader', 'bad')
        self.assertNotEqual(denied.returncode, 0)
        self.assertIn('NOPERM', denied.stdout + denied.stderr)
        self.assertNotEqual(self.as_reader(selected, 'PING', gid=65534).returncode, 0)
        self.assertNotEqual(self.as_reader(legacy, 'PING').returncode, 0)
        parent_acl = subprocess.check_output(['getfacl', '-ncp', str(selected.parent)], text=True)
        self.assertNotIn('group:100:', parent_acl)
        self.assertNotIn('default:', parent_acl)
        # The existing host launcher chmod must preserve the named reader entry.
        selected.chmod(0o760)
        self.assertEqual(self.as_reader(selected, 'PING').returncode, 0)

    def test_reader_access_returns_after_redis_process_recreates_socket(self):
        self.select(redis='enforce')
        self.prepare()
        path = self.root / 'sockets/redis.sock'
        process = self.start_socket_server(path, enforce=True)
        child = int(path.with_suffix('.pid').read_text())
        process.terminate()
        self.assertEqual(process.wait(timeout=5), 0)
        with self.assertRaises(ProcessLookupError):
            os.kill(child, 0)
        self.assertFalse(path.exists())
        self.start_socket_server(path, enforce=True)
        self.assertEqual(self.as_reader(path, 'PING').returncode, 0)
        self.assertEqual(self.as_reader(path, 'PING', gid=1001).returncode, 0)
        self.assertNotEqual(self.as_reader(path, 'PING', gid=65534).returncode, 0)

    def test_socket_acl_failure_stops_the_redis_child(self):
        self.select(redis='enforce')
        self.prepare()
        path = self.root / 'failed.sock'
        pidfile = self.root / 'failed.pid'
        command = ['/usr/bin/redis-server', '--port', '0', '--unixsocket', str(path),
                   '--unixsocketperm', '760', '--save', '', '--appendonly', 'no',
                   '--pidfile', str(pidfile), '--logfile', str(self.root / 'failed.log')]
        child = None

        def fail_acl(*_args, **_kwargs):
            nonlocal child
            child = int(pidfile.read_text())
            raise subprocess.CalledProcessError(1, ['setfacl'])

        with mock.patch.object(server.subprocess, 'run', side_effect=fail_acl):
            with self.assertRaises(subprocess.CalledProcessError):
                server.run(path, command, groups_path=self.runtime / 'server/socket-groups.json')
        self.assertIsNotNone(child)
        with self.assertRaises(ProcessLookupError):
            os.kill(child, 0)

    def test_socket_helper_preserves_early_redis_failure_status(self):
        self.select(redis='enforce')
        self.prepare()
        self.assertEqual(server.run(self.root / 'absent.sock',
                                    [sys.executable, '-c', 'import sys; sys.exit(23)'],
                                    groups_path=self.runtime / 'server/socket-groups.json'), 23)

    def test_socket_reader_group_policy_must_be_safe(self):
        policy = self.root / 'socket-groups.json'
        for value in ([], [True], [0], [-1], [2**32 - 1], ['100'], {'gid': 100}, [100, 101, 102]):
            policy.write_text(json.dumps(value))
            with self.assertRaises(RuntimeError):
                server.reader_acl(policy)
        policy.write_text('[100, 1001]')
        policy.chmod(0o666)
        with self.assertRaises(RuntimeError):
            server.reader_acl(policy)
        policy.chmod(0o644)
        self.assertIn('group:1001:rw-', server.reader_acl(policy))
