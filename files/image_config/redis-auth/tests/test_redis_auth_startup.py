"""Selected-process startup and real Redis read/write checks. Run as root."""

import copy
import importlib.util
import json
import os
from pathlib import Path
import pwd
import shlex
import socket
import stat
import subprocess
import sys
import time
import unittest
from unittest import mock

import jinja2
import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import redis_auth_startup as startup

ROOT = Path(__file__).resolve().parents[4]
spec = importlib.util.spec_from_file_location('redis_auth_configure', ROOT / 'dockers/docker-database/redis_auth_configure.py')
configure = importlib.util.module_from_spec(spec)
spec.loader.exec_module(configure)


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
    @pytest.fixture(autouse=True)
    def setup_runtime(self, root_directory, monkeypatch):
        self.root = root_directory
        self.runtime = self.root / 'run'
        self.intent = self.root / 'intent.json'
        monkeypatch.setattr(startup, 'build_enabled', lambda: True)

    def select(self, **intent):
        self.intent.write_text(json.dumps(intent))
        self.intent.chmod(0o644)

    def prepare(self):
        return startup.prepare_selected(intent_path=self.intent, runtime=self.runtime)

    def enforce(self, **intent):
        self.select(mode='enforce', **intent)
        return self.prepare()

    def test_absent_or_legacy_is_inactive(self):
        self.assertEqual(self.prepare(), [])
        self.select(mode='legacy')
        self.assertEqual(self.prepare(), [])
        self.assertFalse(self.runtime.exists())

    def test_explicit_primary_and_bmp_share_one_boot_pair(self):
        self.select(mode='enforce')
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

    def test_systemd_umask_prepares_both_processes(self):
        self.select(mode='enforce')
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

    def test_world_writable_intent_rejected(self):
        self.select(mode='enforce', tcp_exceptions=['redis_bmp'])
        self.intent.chmod(0o666)
        with self.assertRaises(startup.CredentialStateError):
            self.prepare()

    def test_symlink_intent_rejected(self):
        self.select(mode='enforce', tcp_exceptions=['redis_bmp'])
        link = self.root / 'link'
        link.symlink_to(self.intent)
        with self.assertRaises(OSError):
            startup.prepare_selected(intent_path=link, runtime=self.runtime)

    def test_same_boot_selection_cannot_change(self):
        self.enforce(tcp_exceptions=['redis_bmp'])
        self.select(mode='enforce')
        with self.assertRaises(startup.CredentialStateError):
            self.prepare()
        self.select(mode='legacy')
        with self.assertRaises(startup.CredentialStateError):
            self.prepare()

    def test_startup_check_fails_without_prepared_runtime(self):
        self.select(mode='enforce', tcp_exceptions=['redis_bmp'])
        with self.assertRaises(OSError):
            startup.runtime_required(intent_path=self.intent, runtime=self.runtime)
        self.assertFalse(self.runtime.exists())

    def test_startup_check_rejects_missing_policy_or_acl(self):
        self.enforce(tcp_exceptions=['redis_bmp'])
        self.assertTrue(startup.runtime_required(intent_path=self.intent, runtime=self.runtime))
        (self.runtime / 'local.json').unlink()
        with self.assertRaises(OSError):
            startup.runtime_required(intent_path=self.intent, runtime=self.runtime)
        self.assertFalse((self.runtime / 'local.json').exists())

    def test_startup_check_rejects_changed_acl(self):
        self.enforce(tcp_exceptions=['redis_bmp'])
        (self.runtime / 'server/users.acl').write_text('user default on nopass +@all')
        with self.assertRaises(startup.CredentialStateError):
            startup.runtime_required(intent_path=self.intent, runtime=self.runtime)

    def test_partial_credential_is_not_replaced(self):
        self.enforce(tcp_exceptions=['redis_bmp'])
        (self.runtime / 'local-system/writer.secret').unlink()
        with self.assertRaises(startup.CredentialStateError):
            self.prepare()

    def test_unsafe_server_directory_rejected(self):
        self.enforce(tcp_exceptions=['redis_bmp'])
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
            self.select(mode='enforce')
            self.prepare()
            policy = json.loads((self.runtime / 'local.json').read_text())
        return configure.configure(database or self.database(), additional or self.additional(), policy,
                                   json.loads((self.runtime / 'server/instances.json').read_text()),
                                   environ=environ or {}, chassis=chassis)

    def test_only_selected_process_metadata_changes(self):
        self.enforce(tcp_exceptions=['redis_bmp'])
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
        self.enforce(tcp_exceptions=['redis_bmp'])
        entry = json.loads((self.runtime / 'local.json').read_text())[0]
        with self.assertRaises(ValueError):
            self.configured(policy=[entry, entry])
        entry['tcp_port'] = 6379
        with self.assertRaises(ValueError):
            self.configured(policy=[entry])

    def test_missing_selected_process_and_changed_exception_are_rejected(self):
        self.enforce()
        policy = json.loads((self.runtime / 'local.json').read_text())
        with self.assertRaises(ValueError):
            self.configured(policy=policy[:1])
        self.select(mode='legacy')
        # Use a separate boot directory for the deliberately mixed selection.
        self.runtime = self.root / 'mixed-run'
        self.enforce(tcp_exceptions=['redis_bmp'])
        database = self.database()
        database['INSTANCES']['redis_bmp']['port'] += 1
        with self.assertRaises(ValueError):
            self.configured(database=database, policy=json.loads((self.runtime / 'local.json').read_text()))

    def test_redis_uid_reads_acl_but_not_password(self):
        self.enforce(tcp_exceptions=['redis_bmp'])
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
        template = jinja2.Template((ROOT / 'dockers/docker-database/supervisord.conf.j2').read_text())
        database, additional = self.database(), self.additional()
        self.enforce(tcp_exceptions=['redis_bmp'])
        self.configured(database, additional, json.loads((self.runtime / 'local.json').read_text()))
        for name, extra in additional['INSTANCES'].items():
            database['INSTANCES'][name].update(extra)
        rendered = template.render(**database)
        commands = [line for line in rendered.splitlines() if 'exec ' in line and '/usr/bin/redis-server' in line]
        self.assertEqual(len(commands), 2)
        selected = next(line for line in commands if '--unixsocket /var/run/redis/redis.sock ' in line)
        legacy = next(line for line in commands if '--unixsocket /var/run/redis/redis_bmp.sock ' in line)
        self.assertIn('--port 0 ', selected)
        self.assertIn('exec /usr/bin/redis-server', selected)
        self.assertIn('--unixsocketperm 766 --aclfile /run/redis-auth/server/users.acl', selected)
        self.assertNotIn('--aclfile', legacy)

    def render_launcher(self, name):
        environment = jinja2.Environment(undefined=jinja2.ChainableUndefined)
        return environment.from_string((ROOT / 'files/build_templates/docker_image_ctl.j2').read_text()).render(
            docker_container_name=name, sonic_asic_platform='vs', docker_image_run_opt='',
            docker_image_name='test', install_debug_image='n', mount_default_tmpfs='n')

    def test_container_control_template_is_valid_bash(self):
        for name in ('database', 'swss', 'pmon', 'bgp', 'snmp'):
            rendered = self.render_launcher(name)
            result = subprocess.run(['bash', '-n'], input=rendered, text=True, capture_output=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertIn('REDIS_AUTH_REQUIRED=1', rendered)
            if name == 'database':
                self.assertNotIn('redis_socket_mode', rendered)
                for socket in ('REDIS_BMP_SOCK', 'REDIS_SOCK'):
                    self.assertIn(f'set_redis_socket_permissions \"${socket}\"', rendered)
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
        for name in ('database', 'swss', 'pmon', 'bgp', 'snmp'):
            rendered = self.render_launcher(name)
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
        self.select(mode='enforce')
        previous = os.umask(0o077)
        try:
            self.prepare()
        finally:
            os.umask(previous)
        for name in startup.configured_instances():
            path = self.root / 'sockets' / (name + '.sock')
            process = self.start_socket_server(path, enforce=True, databases=100)
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
            self.wait_for_reader_socket(path, process)
            self.assertEqual(stat.S_IMODE(path.stat().st_mode), 0o766)
            # Named writers retain existing administrative commands.
            self.assertEqual(writer.command('CONFIG', 'SET', 'timeout', 0), b'OK')

    def wait_for_reader_socket(self, path, process):
        for _ in range(200):
            self.assertIsNone(process.poll(), 'Redis exited')
            if path.exists() and stat.S_IMODE(path.stat().st_mode) == 0o766:
                return
            time.sleep(0.02)
        self.fail('selected socket never granted reader access')

    def start_socket_server(self, path, *, enforce, port=0, databases=16):
        identity = pwd.getpwnam('redis')
        path.parent.mkdir(exist_ok=True)
        path.parent.chmod(0o755)
        os.chown(path.parent, identity.pw_uid, identity.pw_gid)
        command = ['/usr/bin/redis-server', '--port', str(port), '--bind', '127.0.0.1', '--unixsocket', str(path),
                   '--unixsocketperm', '766' if enforce else '760', '--save', '', '--appendonly', 'no',
                   '--pidfile', str(path.with_suffix('.pid')), '--databases', str(databases)]
        if enforce:
            command += ['--aclfile', str(self.runtime / 'server/users.acl')]

        def demote():
            os.setgroups([])
            os.setgid(identity.pw_gid)
            os.setuid(identity.pw_uid)

        process = subprocess.Popen(command, preexec_fn=demote,
                                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        self.addCleanup(lambda: process.wait(timeout=5) if process.poll() is not None
                        else (process.terminate(), process.wait(timeout=5)))
        for _ in range(200):
            self.assertIsNone(process.poll(), 'Redis exited')
            if path.exists():
                break
            time.sleep(0.02)
        if enforce:
            self.wait_for_reader_socket(path, process)
        return process

    def as_reader(self, path, *command, gid=100):
        def demote():
            os.setgroups([])
            os.setgid(gid)
            os.setuid(65534)

        return subprocess.run(['redis-cli', '-s', str(path), '--raw', '-e', *command],
                              preexec_fn=demote, text=True, capture_output=True)

    def apply_host_socket_permissions(self, path, *, selected=None, mode='enforce', malformed=False):
        rendered = self.render_launcher('database')
        start = rendered.index('set_redis_socket_permissions() {')
        end = rendered.index('\n}', start) + 2
        policy = self.root / 'host-socket-policy.json'
        policy.write_text('invalid JSON' if malformed else json.dumps(
            [{'unix_socket': str(selected)}] if selected is not None else []))
        function = rendered[start:end].replace('/run/redis-auth/local.json', str(policy))
        script = (function + '\nREDIS_AUTH_MODE=' + shlex.quote(mode) +
                  '\nset_redis_socket_permissions ' + shlex.quote(str(path)))
        result = subprocess.run(['bash', '-c', script], text=True, capture_output=True)
        if not malformed or mode == 'legacy':
            self.assertEqual(result.returncode, 0, result.stderr)
        return result

    def test_private_group_account_reads_without_writer_credentials(self):
        self.enforce()
        path = self.root / 'sockets/redis.sock'
        self.start_socket_server(path, enforce=True)
        writer = Redis(path)
        self.addCleanup(writer.close)
        secret = self.runtime / 'local-system/writer.secret'
        writer.command('AUTH', startup.USERNAME, secret.read_text().strip())
        for database in (0, 1, 4, 15):
            writer.command('SELECT', database)
            self.assertEqual(writer.command('SET', 'test:reader', 'kept'), b'OK')
            result = self.as_reader(path, '-n', str(database), 'GET', 'test:reader', gid=1002)
            self.assertEqual((result.returncode, result.stdout.strip()), (0, 'kept'))
        for command in (('SET', 'test:reader', 'bad'), ('DEL', 'test:reader'), ('FLUSHDB',),
                        ('CONFIG', 'SET', 'timeout', '10'),
                        ('EVAL', "return redis.call('SET',KEYS[1],ARGV[1])", '1', 'test:reader', 'bad')):
            denied = self.as_reader(path, *command, gid=1002)
            self.assertNotEqual(denied.returncode, 0)
            expected = 'ACL failure in script' if command[0] == 'EVAL' else 'NOPERM'
            self.assertIn(expected, denied.stdout + denied.stderr)
        self.assertEqual(self.as_reader(path, 'GET', 'test:reader', gid=1002).stdout.strip(), 'kept')

        def ordinary():
            os.setgroups([])
            os.setgid(1002)
            os.setuid(65534)

        result = subprocess.run(['cat', str(secret)], preexec_fn=ordinary,
                                capture_output=True, text=True)
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(result.stdout, '')
        self.assertEqual(stat.S_IMODE(secret.stat().st_mode), 0o400)

    def test_host_socket_permissions_reject_unreadable_enforcement_policy(self):
        self.enforce()
        path = self.root / 'sockets/legacy.sock'
        self.start_socket_server(path, enforce=False)
        result = self.apply_host_socket_permissions(path, malformed=True)
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(stat.S_IMODE(path.stat().st_mode), 0o760)
        self.assertNotEqual(self.as_reader(path, 'PING', gid=1002).returncode, 0)

    def test_legacy_host_socket_permissions_ignore_enforcement_policy(self):
        path = self.root / 'sockets/legacy.sock'
        self.start_socket_server(path, enforce=False)
        self.apply_host_socket_permissions(path, mode='legacy', malformed=True)
        self.assertEqual(stat.S_IMODE(path.stat().st_mode), 0o760)
        self.assertNotEqual(self.as_reader(path, 'PING', gid=1002).returncode, 0)

    def test_mixed_sockets_admit_existing_readers_without_opening_legacy(self):
        self.enforce(tcp_exceptions=['redis_bmp'])
        selected, legacy = self.root / 'sockets/redis.sock', self.root / 'sockets/redis_bmp.sock'
        self.start_socket_server(selected, enforce=True)
        self.start_socket_server(legacy, enforce=False)
        for gid in (27, 100, 1001, 1002, 65534, pwd.getpwnam('redis').pw_gid):
            result = self.as_reader(selected, 'PING', gid=gid)
            self.assertEqual((result.returncode, result.stdout.strip()), (0, 'PONG'))
        for gid in (27, 100, 1001, 1002, 65534):
            denied = self.as_reader(selected, 'SET', 'test:reader', 'bad', gid=gid)
            self.assertNotEqual(denied.returncode, 0)
            self.assertIn('NOPERM', denied.stdout + denied.stderr)
        self.assertNotEqual(self.as_reader(legacy, 'PING').returncode, 0)
        self.assertNotIn('system.posix_acl_default', os.listxattr(selected.parent))
        # The host launcher must preserve admission for private-group accounts.
        self.apply_host_socket_permissions(selected, selected=selected)
        self.apply_host_socket_permissions(legacy, selected=selected)
        self.assertEqual(stat.S_IMODE(selected.stat().st_mode), 0o766)
        self.assertEqual(stat.S_IMODE(legacy.stat().st_mode), 0o760)
        self.assertEqual(self.as_reader(selected, 'PING', gid=1002).returncode, 0)
        self.assertNotEqual(self.as_reader(legacy, 'PING', gid=1002).returncode, 0)

    def test_mixed_mode_tcp_compatibility_subscriptions_and_selected_restart(self):
        with socket.socket() as reservation:
            reservation.bind(('127.0.0.1', 0))
            port = reservation.getsockname()[1]
        database = self.database()
        database['INSTANCES']['redis_bmp']['port'] = port
        (startup.CONFIG / 'database_config.json').write_text(json.dumps(database))
        self.enforce(tcp_exceptions=['redis_bmp'])
        policy = json.loads((self.runtime / 'local.json').read_text())
        self.configured(database, self.additional(), policy)
        self.assertEqual(database['INSTANCES']['redis']['port'], 0)
        self.assertEqual(database['INSTANCES']['redis_bmp']['port'], port)
        selected, legacy = self.root / 'sockets/redis.sock', self.root / 'sockets/redis_bmp.sock'
        process = self.start_socket_server(selected, enforce=True)
        self.start_socket_server(legacy, enforce=False, port=port)
        secret = self.runtime / 'local-system/writer.secret'
        inode, password = secret.stat().st_ino, secret.read_bytes()

        def subscription_check():
            writer, subscriber = Redis(selected), Redis(selected)
            try:
                self.assertEqual(writer.command('AUTH', startup.USERNAME, password), b'OK')
                self.assertEqual(writer.command('CONFIG', 'GET', 'port'), [b'port', b'0'])
                self.assertEqual(writer.command('SET', 'test:key', 'before'), b'OK')
                self.assertEqual(subscriber.command('SUBSCRIBE', 'test:channel'),
                                 [b'subscribe', b'test:channel', 1])
                self.assertEqual(writer.command('PUBLISH', 'test:channel', 'value'), 1)
                self.assertEqual(subscriber.reply(), [b'message', b'test:channel', b'value'])
            finally:
                writer.close()
                subscriber.close()
            self.assertEqual(self.as_reader(selected, 'GET', 'test:key').stdout.strip(), 'before')
            denied = self.as_reader(selected, 'SET', 'test:key', 'bad')
            self.assertNotEqual(denied.returncode, 0)
            self.assertIn('NOPERM', denied.stdout + denied.stderr)

        def ordinary():
            os.setgroups([])
            os.setgid(65534)
            os.setuid(65534)

        legacy_write = subprocess.run(['redis-cli', '-h', '127.0.0.1', '-p', str(port),
                                       '-e', 'SET', 'test:legacy', 'allowed'],
                                      preexec_fn=ordinary, text=True, capture_output=True)
        self.assertEqual((legacy_write.returncode, legacy_write.stdout.strip()), (0, 'OK'))
        subscription_check()
        process.terminate()
        self.assertEqual(process.wait(timeout=5), 0)
        self.start_socket_server(selected, enforce=True)
        self.prepare()
        self.assertEqual(secret.stat().st_ino, inode)
        subscription_check()

    def test_reader_access_returns_after_redis_process_recreates_socket(self):
        self.enforce(tcp_exceptions=['redis_bmp'])
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
        self.assertEqual(self.as_reader(path, 'PING', gid=27).returncode, 0)
        self.assertEqual(self.as_reader(path, 'PING', gid=65534).returncode, 0)
        self.assertEqual(self.as_reader(path, 'PING', gid=1002).returncode, 0)
        denied = self.as_reader(path, 'SET', 'test:reader', 'bad', gid=1002)
        self.assertNotEqual(denied.returncode, 0)
        self.assertIn('NOPERM', denied.stdout + denied.stderr)
