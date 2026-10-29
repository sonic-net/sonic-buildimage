#!/usr/bin/env python3
"""Prepare the explicitly selected local Redis processes before database startup."""

import argparse
import json
import os
from pathlib import Path
import stat
import sys
import tempfile

from redis_auth_prepare import CredentialStateError, prepare_credential

INTENT = Path('/etc/sonic/redis-auth.json')
RUNTIME = Path('/run/redis-auth')
BUILD_SETTING = Path('/usr/share/sonic/redis-auth/build-enabled')
CONFIG = Path('/etc/sonic')
DATABASE_TEMPLATE = Path('/usr/share/sonic/redis-auth/database_config.json.j2')
USERNAME = 'sonic-trusted-writer'
READ_RULES = ('reset on nopass ~* &* -@all +@read +@transaction '
              '+ping +echo +select +quit +hello +client|setname '
              '+client|getname +client|id +client|info '
              '+info +command +role +time +lastsave +config|get '
              '+acl|whoami +subscribe +psubscribe +ssubscribe '
              '+unsubscribe +punsubscribe +sunsubscribe +pubsub '
              '+eval +evalsha +eval_ro +evalsha_ro +script|exists +script|load')


def read_setting(path):
    """Read a regular, root-controlled startup setting."""
    fd = os.open(path, os.O_RDONLY | os.O_NONBLOCK | os.O_NOFOLLOW | os.O_CLOEXEC)
    with os.fdopen(fd) as stream:
        info = os.fstat(stream.fileno())
        if (not stat.S_ISREG(info.st_mode) or info.st_uid != 0
                or info.st_mode & 0o022):
            raise CredentialStateError('unsafe Redis activation setting')
        return stream.read()


def build_enabled():
    try:
        value = read_setting(BUILD_SETTING).strip()
    except FileNotFoundError:
        return False
    if value not in ('y', 'n'):
        raise CredentialStateError('invalid Redis build setting')
    return value == 'y'


def read_intent(path=INTENT):
    """Read the common activation mode and optional TCP exceptions."""
    try:
        intent = json.loads(read_setting(path))
    except FileNotFoundError:
        intent = {}
    if not isinstance(intent, dict):
        raise CredentialStateError('expected a Redis activation object')
    exceptions = intent.get('tcp_exceptions', [])
    if (set(intent) - {'mode', 'tcp_exceptions'} or intent.get('mode', 'legacy') not in ('legacy', 'enforce')
            or not isinstance(exceptions, list) or any(not isinstance(name, str) or not name for name in exceptions)
            or len(exceptions) != len(set(exceptions))):
        raise CredentialStateError('expected mode legacy or enforce and distinct TCP exception names')
    return {'mode': intent.get('mode', 'legacy'), 'tcp_exceptions': sorted(exceptions)}


def configured_instances():
    """Use the same saved override or standard host template as the database entrypoint."""
    if (CONFIG / 'enable_multidb').exists():
        raise CredentialStateError('the multi-database layout is not qualified for enforcement')
    try:
        override = CONFIG / 'old_config/database_config.json'
        if not os.path.lexists(override):
            override = CONFIG / 'database_config.json'
        database = json.loads(read_setting(override))
    except FileNotFoundError:
        import jinja2
        database = json.loads(jinja2.Template(read_setting(DATABASE_TEMPLATE)).render(
            HOST_IP='127.0.0.1', REDIS_PORT=6379, BMP_DB_PORT=6400, DEV='', DATABASE_TYPE=''))
    instances = dict(database['INSTANCES'])
    # The entrypoint removes this reference before launching standalone servers.
    instances.pop('redis_chassis', None)
    if not instances:
        raise CredentialStateError('no configured local Redis processes')
    sockets = set()
    for name, instance in instances.items():
        path = instance['unix_socket_path']
        if (name == 'remote_redis' or instance['hostname'] != '127.0.0.1' or instance.get('database_type')
                or instance.get('bmc_link_ip') or not isinstance(path, str)
                or Path(path).parent != Path('/var/run/redis') or '..' in Path(path).parts
                or path in sockets or type(instance['port']) is not int or not 0 <= instance['port'] <= 65535):
            raise CredentialStateError('Redis inventory requires distinct qualified local endpoints')
        sockets.add(path)
    return {name: {key: instance[key] for key in ('hostname', 'port', 'unix_socket_path')}
            for name, instance in sorted(instances.items())}


def selection(path=INTENT):
    intent = read_intent(path)
    if intent['mode'] == 'legacy':
        return [], {}
    instances = configured_instances()
    exceptions = set(intent['tcp_exceptions'])
    if exceptions - instances.keys() or any(instances[name]['port'] == 0 for name in exceptions):
        raise CredentialStateError('TCP exceptions must name configured TCP processes')
    selected = sorted(instances.keys() - exceptions)
    if not selected:
        raise CredentialStateError('enforcement must select at least one local Redis process')
    for name, instance in instances.items():
        instance['protected'] = name in selected
    return selected, instances


def selected_processes(path=INTENT):
    return selection(path)[0]


def endpoint_policy(selected, instances):
    policy = [{'username': USERNAME, 'unix_socket': instances[name]['unix_socket_path'],
               'tcp_host': instances[name]['hostname'], 'tcp_port': 0} for name in selected]
    if len((json.dumps(policy, indent=2) + '\n').encode()) > 4096:
        raise CredentialStateError('generated endpoint policy exceeds the installed reader capacity')
    return policy


def atomic(path, content, mode=0o644):
    fd, temporary = tempfile.mkstemp(prefix='.' + path.name + '-', dir=path.parent)
    try:
        with os.fdopen(fd, 'wb') as stream:
            os.fchmod(stream.fileno(), mode)
            stream.write(content)
        os.replace(temporary, path)
    finally:
        if os.path.exists(temporary):
            os.unlink(temporary)


def prepare_selected(*, intent_path=INTENT, runtime=RUNTIME):
    if os.geteuid() != 0:
        raise CredentialStateError('must run as root')
    selected, instances = selection(intent_path) if build_enabled() else ([], {})
    if not selected:
        # Changing activation requires a controlled host reboot, not a live flip.
        if (runtime / 'local.json').exists():
            raise CredentialStateError('activation changed during this boot; reboot after changing intent')
        return []
    policy = endpoint_policy(selected, instances)
    policy_bytes = (json.dumps(policy, indent=2) + '\n').encode()
    existing = runtime / 'local.json'
    if existing.exists() and existing.read_bytes() != policy_bytes:
        raise CredentialStateError('activation changed during this boot; reboot after changing intent')
    inventory = runtime / 'server/instances.json'
    inventory_bytes = (json.dumps(instances, indent=2) + '\n').encode()
    if inventory.exists() and inventory.read_bytes() != inventory_bytes:
        raise CredentialStateError('Redis inventory changed during this boot; reboot required')
    prepare_credential(runtime_root=runtime)
    digest = (runtime / 'local-system/writer.sha256').read_text().strip()
    server = runtime / 'server'
    try:
        server.mkdir(mode=0o755)
    except FileExistsError:
        pass
    else:
        server.chmod(0o755)
    info = server.lstat()
    if (not stat.S_ISDIR(info.st_mode) or info.st_uid != 0
            or stat.S_IMODE(info.st_mode) != 0o755):
        raise CredentialStateError('unsafe Redis server runtime directory')
    acl = ('user default ' + READ_RULES + '\nuser ' + USERNAME
           + ' reset on #' + digest + ' ~* &* +@all\n')
    atomic(server / 'users.acl', acl.encode())
    atomic(inventory, inventory_bytes)
    atomic(existing, policy_bytes)
    return selected


def runtime_required(*, intent_path=INTENT, runtime=RUNTIME):
    selected, instances = selection(intent_path) if build_enabled() else ([], {})
    if not selected:
        if (runtime / 'local.json').exists():
            raise CredentialStateError('activation changed during this boot')
        return False
    for relative in ('local.json', 'local-system/writer.secret', 'local-system/writer.sha256',
                     'server/users.acl', 'server/instances.json'):
        info = (runtime / relative).lstat()
        if (not stat.S_ISREG(info.st_mode) or info.st_uid != 0 or info.st_mode & 0o022):
            raise CredentialStateError('missing or unsafe selected runtime file')
    # The complete published pair must already exist. Checking cannot initialize it.
    prepare_credential(runtime_root=runtime)
    expected = endpoint_policy(selected, instances)
    if json.loads((runtime / 'local.json').read_text()) != expected:
        raise CredentialStateError('runtime policy differs from activation setting')
    if json.loads(read_setting(runtime / 'server/instances.json')) != instances:
        raise CredentialStateError('runtime inventory differs from configured local processes')
    digest = (runtime / 'local-system/writer.sha256').read_text().strip()
    expected_acl = ('user default ' + READ_RULES + '\nuser ' + USERNAME
                    + ' reset on #' + digest + ' ~* &* +@all\n')
    if (runtime / 'server/users.acl').read_text() != expected_acl:
        raise CredentialStateError('server policy differs from activation setting')
    return True


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true', help='validate prepared runtime without initializing it')
    args = parser.parse_args()
    try:
        if args.check:
            print('enforce' if runtime_required() else 'legacy')
            return 0
        selected = prepare_selected()
    except (CredentialStateError, OSError, ValueError, KeyError, TypeError):
        print('redis-auth-startup: invalid activation or runtime state', file=sys.stderr)
        return 1
    message = 'legacy'
    if selected:
        inventory = json.loads(read_setting(RUNTIME / 'server/instances.json'))
        exceptions = sorted(name for name, item in inventory.items() if not item['protected'])
        message = 'protected=' + ','.join(selected) + ' unprotected_tcp=' + (','.join(exceptions) or 'none')
    print('redis-auth-startup: ' + message)
    return 0


if __name__ == '__main__':
    sys.exit(main())
