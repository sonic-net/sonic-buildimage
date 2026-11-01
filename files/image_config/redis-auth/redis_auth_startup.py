#!/usr/bin/env python3
"""Prepare the explicitly selected local Redis processes before database startup.

Topology extensions qualify a deployment and discover its owned servers. Selection,
credentials and runtime delivery consume that inventory through the common path.
Remote references are not owned servers; shared TCP servers require an explicit
exception and matching client/restore support before their topology is admitted.
"""

import argparse
import configparser
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
NETNS = Path('/run/netns')
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
    value = read_setting(BUILD_SETTING).strip()
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


def platform_directory():
    from sonic_py_common import device_info
    platform = device_info.get_platform(config_db=None)
    if not platform or platform in ('.', '..') or Path(platform).name != platform:
        raise CredentialStateError('unknown Redis platform identity')
    directory = Path(device_info.HOST_DEVICE_PATH) / platform
    alias = Path(device_info.CONTAINER_PLATFORM_PATH)
    if not directory.is_dir() or (os.path.lexists(alias) and alias.resolve() != directory.resolve()):
        raise CredentialStateError('missing or conflicting Redis platform directory')
    return directory


def platform_settings(path):
    parser = configparser.ConfigParser(interpolation=None)
    parser.read_string('[platform]\n' + read_setting(path))
    if parser.sections() != ['platform'] or parser.defaults():
        raise CredentialStateError('unsupported platform assignment format')
    return parser['platform']


def platform_layout():
    directory = platform_directory()
    asic = directory / 'asic.conf'
    count = int(platform_settings(asic)['num_asic']) if os.path.lexists(asic) else 1
    if count < 1:
        raise CredentialStateError('invalid ASIC count')
    return directory, [''] + ([str(index) for index in range(count)] if count > 1 else [])


def effective_override(name):
    saved = CONFIG / 'old_config' / name
    return saved if os.path.lexists(saved) else CONFIG / name


def check_topology():
    """Qualify a standalone NPU host and return its host/ASIC layout before startup.

    New topologies extend this boundary and configured_instances together. They
    must establish ownership and remote-client requirements before enforcement.
    """
    directory, numbers = platform_layout()
    source = CONFIG / ('old_config' if os.path.lexists(CONFIG / 'pending_config_migration') else '')
    metadata = json.loads(read_setting(source / 'config_db.json'))['DEVICE_METADATA']['localhost']
    environment, platform = directory / 'platform_env.conf', directory / 'platform.json'
    flags = platform_settings(environment) if os.path.lexists(environment) else {}
    data = json.loads(read_setting(platform)) if os.path.lexists(platform) else {}
    roles = {'ToRRouter', 'LeafRouter', 'BackEndToRRouter', 'BackEndLeafRouter',
             'ToRSwitch', 'MgmtToRRouter', 'MgmtLeafRouter', 'MgmtAccessRouter'}
    if (not isinstance(metadata, dict) or metadata.get('platform') != directory.name
            or metadata.get('switch_type', 'npu') != 'npu' or metadata.get('type') not in roles
            or metadata.get('subtype') not in (None, 'DualToR')
            or any(flags.get(key, '0') != '0' for key in ('supervisor', 'disaggregated_chassis', 'switch_bmc'))
            or not isinstance(data, dict) or 'DPU' in data or 'DPUS' in data
            or os.path.lexists(directory / 'chassisdb.conf')
            or any(os.path.lexists(base / name) for base in (CONFIG, CONFIG / 'old_config')
                   for name in ('chassisdb_address', 'enable_multidb'))
            or any(list(base.glob('database_config*dpu*.json')) for base in (CONFIG, CONFIG / 'old_config'))):
        raise CredentialStateError('unsupported Redis deployment or namespace layout')
    allowed = {prefix + number + '.json' for number in numbers for prefix in ('config_db', 'database_config')}
    expected = {'asic' + number for number in numbers if number}
    if (not {path.name for path in NETNS.glob('asic*')} <= expected
            or any(path.name not in allowed for base in (CONFIG, CONFIG / 'old_config')
                   for prefix in ('config_db', 'database_config') for path in base.glob(prefix + '[0-9]*.json'))):
        raise CredentialStateError('unsupported Redis namespace layout')
    global_config = effective_override('database_global.json')
    includes = [{'include': '../../redis' + number + '/sonic-db/database_config.json',
                 **({'namespace': 'asic' + number} if number else {})} for number in numbers]
    if (os.path.lexists(global_config) and json.loads(read_setting(global_config))['INCLUDES'] !=
            includes):
        raise CredentialStateError('unsupported Redis global database includes')
    return numbers


def configured_instances(numbers=None):
    """Discover owned local servers using the entrypoint's saved/current inputs.

    Return one endpoint record per owned process. Namespace support extends this
    inventory; it does not add credential, ACL or activation paths. References to
    another host never enter it. Shared servers need reviewed TCP exceptions.
    """
    if any(os.path.lexists(base / 'enable_multidb') for base in (CONFIG, CONFIG / 'old_config')):
        raise CredentialStateError('the multi-database layout is not qualified for enforcement')
    numbers = platform_layout()[1] if numbers is None else numbers
    result, sockets = {}, set()
    for number in numbers:
        try:
            database = json.loads(read_setting(effective_override('database_config' + number + '.json')))
        except FileNotFoundError:
            import jinja2
            database = json.loads(jinja2.Template(read_setting(DATABASE_TEMPLATE)).render(
                HOST_IP='127.0.0.1', REDIS_PORT=6379, BMP_DB_PORT=6400, DEV=number, DATABASE_TYPE=''))
        instances = dict(database['INSTANCES'])
        # The entrypoint removes this unused standalone template reference.
        instances.pop('redis_chassis', None)
        if not instances or (number and set(instances) != {'redis', 'redis_bmp'}):
            raise CredentialStateError('expected the standard local Redis namespace layout')
        for name, instance in sorted(instances.items()):
            path = instance['unix_socket_path']
            if (not isinstance(name, str) or '/' in name or name == 'remote_redis'
                    or instance['hostname'] != '127.0.0.1' or instance.get('database_type')
                    or instance.get('bmc_link_ip') or not isinstance(path, str)
                    or Path(path).parent != Path('/var/run/redis' + number) or '..' in Path(path).parts
                    or path in sockets or type(instance['port']) is not int or not 0 <= instance['port'] <= 65535):
                raise CredentialStateError('Redis inventory requires distinct qualified local endpoints')
            sockets.add(path)
            result[('asic' + number + '/' if number else '') + name] = {
                key: instance[key] for key in ('hostname', 'port', 'unix_socket_path')}
    return result


def selection(path=INTENT, *, runtime=None):
    intent = read_intent(path)
    if intent['mode'] == 'legacy':
        return [], {}
    numbers = check_topology() if runtime is not None and not (runtime / 'local.json').exists() else None
    instances = configured_instances(numbers)
    exceptions = set(intent['tcp_exceptions'])
    names = {identity.rsplit('/', 1)[-1] for identity in instances}
    if exceptions - names or any(item['port'] == 0 for identity, item in instances.items()
                                 if identity.rsplit('/', 1)[-1] in exceptions):
        raise CredentialStateError('TCP exceptions must name configured TCP processes')
    selected = sorted(identity for identity in instances if identity.rsplit('/', 1)[-1] not in exceptions)
    if not selected:
        raise CredentialStateError('enforcement must select at least one local Redis process')
    for name, instance in instances.items():
        instance['protected'] = name in selected
    return selected, instances


def policy_files(selected, instances):
    policy = [{'username': USERNAME, 'unix_socket': instances[name]['unix_socket_path'],
               'tcp_host': instances[name]['hostname'], 'tcp_port': 0} for name in selected]
    files = {name: (json.dumps(value, indent=2) + '\n').encode() for name, value in
             (('server/instances.json', instances), ('local.json', policy))}
    if len(files['local.json']) > 4096:
        raise CredentialStateError('generated endpoint policy exceeds the installed reader capacity')
    return files


def server_directory(runtime, *, create=False):
    path = runtime / 'server'
    if create:
        try:
            path.mkdir(mode=0o755)
        except FileExistsError:
            pass
        else:
            path.chmod(0o755)
    info = path.lstat()
    if (not stat.S_ISDIR(info.st_mode) or info.st_uid != 0
            or stat.S_IMODE(info.st_mode) != 0o755):
        raise CredentialStateError('unsafe Redis server runtime directory')


def server_acl(digest):
    return ('user default ' + READ_RULES + '\nuser ' + USERNAME
            + ' reset on #' + digest + ' ~* &* +@all\n').encode()


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
    selected, instances = selection(intent_path, runtime=runtime) if build_enabled() else ([], {})
    if not selected:
        # Changing activation requires a controlled host reboot, not a live flip.
        if (runtime / 'local.json').exists():
            raise CredentialStateError('activation changed during this boot; reboot after changing intent')
        return []
    files = policy_files(selected, instances)
    for relative, content in files.items():
        path = runtime / relative
        if path.exists() and read_setting(path).encode() != content:
            raise CredentialStateError('activation changed during this boot; inventory or policy differs; reboot required')
    digest = prepare_credential(runtime_root=runtime)
    server_directory(runtime, create=True)
    for relative, content in {'server/users.acl': server_acl(digest), **files}.items():
        atomic(runtime / relative, content)
    return selected


def runtime_required(*, intent_path=INTENT, runtime=RUNTIME):
    selected, instances = selection(intent_path) if build_enabled() else ([], {})
    if not selected:
        if (runtime / 'local.json').exists():
            raise CredentialStateError('activation changed during this boot')
        return False
    server_directory(runtime)
    files = policy_files(selected, instances)
    # Check public files before credentials; this path must never initialize state.
    for relative, content in files.items():
        if json.loads(read_setting(runtime / relative)) != json.loads(content):
            raise CredentialStateError('runtime inventory or policy differs from activation setting')
    digest = prepare_credential(runtime_root=runtime, create=False)
    if read_setting(runtime / 'server/users.acl').encode() != server_acl(digest):
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
    except (CredentialStateError, OSError, ValueError, KeyError, TypeError, configparser.Error):
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
