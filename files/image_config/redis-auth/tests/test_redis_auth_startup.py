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

    def select(self, **modes):
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
        self.assertEqual({entry['unix_socket'] for entry in policy}, set(startup.SOCKETS.values()))
        self.assertTrue(all(entry['tcp_port'] == 0 for entry in policy))
        secret = self.runtime / 'local-system/writer.secret'
        inode = secret.stat().st_ino
        self.prepare()
        self.assertEqual(secret.stat().st_ino, inode)
        self.assertEqual(stat.S_IMODE(secret.stat().st_mode), 0o400)
        self.assertEqual(stat.S_IMODE((self.runtime / 'local.json').stat().st_mode), 0o644)
        self.assertEqual(stat.S_IMODE((self.runtime / 'server/users.acl').stat().st_mode), 0o644)

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
        return {'INSTANCES': {name: {'hostname': '127.0.0.1', 'port': 6379 + number,
                                    'unix_socket_path': path}
                              for number, (name, path) in enumerate(startup.SOCKETS.items())}}

    def additional(self):
        return {'INSTANCES': {name: {'is_protected_mode': True} for name in startup.SOCKETS}}

    def configured(self, database=None, additional=None, policy=None, environ=None, chassis=False):
        if policy is None:
            self.select(redis='enforce', redis_bmp='enforce')
            self.prepare()
            policy = json.loads((self.runtime / 'local.json').read_text())
        return configure.configure(database or self.database(), additional or self.additional(), policy,
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
        commands = [line for line in rendered.splitlines() if 'exec /usr/bin/redis-server' in line]
        self.assertEqual(len(commands), 2)
        selected = next(line for line in commands if '--unixsocket /var/run/redis/redis.sock ' in line)
        legacy = next(line for line in commands if '--unixsocket /var/run/redis/redis_bmp.sock ' in line)
        self.assertIn('--port 0 ', selected)
        self.assertIn('--unixsocketperm 666 --aclfile /run/redis-auth/server/users.acl', selected)
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
            if name == 'database':
                self.assertIn('REDIS_AUTH_REQUIRED=1', rendered)
                self.assertIn('redis-auth-delivery-missing', rendered)
                self.assertIn('redis_auth_startup.py --check', rendered)

    def test_database_container_reuse_matches_current_boot_mode(self):
        import jinja2
        environment = jinja2.Environment(undefined=jinja2.ChainableUndefined)
        template = environment.from_string((ROOT / 'files/build_templates/docker_image_ctl.j2').read_text())
        rendered = template.render(docker_container_name='database', sonic_asic_platform='vs',
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
            self.assertEqual(result.stdout, expected, (mode, mounts, marker))

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
        for name in startup.SOCKETS:
            path = socket_dir / (name + '.sock')
            process = subprocess.Popen(['redis-server', '--port', '0', '--unixsocket', str(path),
                                        '--unixsocketperm', '666', '--save', '', '--appendonly', 'no',
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
            self.assertEqual(reader.command('CLIENT', 'SETINFO', 'LIB-NAME', 'ordinary-reader'), b'OK')
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
            self.assertEqual(stat.S_IMODE(path.stat().st_mode), 0o666)
            # Named writers retain existing administrative commands.
            self.assertEqual(writer.command('CONFIG', 'SET', 'timeout', 0), b'OK')
