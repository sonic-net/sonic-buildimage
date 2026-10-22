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
SOCKETS = {'redis': '/var/run/redis/redis.sock',
           'redis_bmp': '/var/run/redis/redis_bmp.sock'}
USERNAME = 'sonic-trusted-writer'
READ_RULES = ('reset on nopass ~* &* -@all +@read +@transaction '
              '+ping +echo +select +quit +hello +client|setname '
              '+client|getname +client|id +client|info +client|setinfo '
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


def selected_processes(path=INTENT):
    """Read root-controlled boot intent. An absent setting keeps legacy mode."""
    try:
        intent = json.loads(read_setting(path))
    except FileNotFoundError:
        return []
    if (not isinstance(intent, dict) or set(intent) - SOCKETS.keys()
            or any(mode not in ('legacy', 'enforce') for mode in intent.values())):
        raise CredentialStateError('expected local Redis process names with legacy or enforce mode')
    return sorted(name for name, mode in intent.items() if mode == 'enforce')


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
    selected = selected_processes(intent_path) if build_enabled() else []
    if not selected:
        # Changing activation requires a controlled host reboot, not a live flip.
        if (runtime / 'local.json').exists():
            raise CredentialStateError('activation changed during this boot; reboot after changing intent')
        return []
    prepare_credential(runtime_root=runtime)
    policy = [{'username': USERNAME, 'unix_socket': SOCKETS[name],
               'tcp_host': '127.0.0.1', 'tcp_port': 0} for name in selected]
    policy_bytes = (json.dumps(policy, indent=2) + '\n').encode()
    existing = runtime / 'local.json'
    if existing.exists() and existing.read_bytes() != policy_bytes:
        raise CredentialStateError('activation changed during this boot; reboot after changing intent')
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
    atomic(existing, policy_bytes)
    return selected


def runtime_required(*, intent_path=INTENT, runtime=RUNTIME):
    selected = selected_processes(intent_path) if build_enabled() else []
    if not selected:
        if (runtime / 'local.json').exists():
            raise CredentialStateError('activation changed during this boot')
        return False
    for relative in ('local.json', 'local-system/writer.secret', 'local-system/writer.sha256', 'server/users.acl'):
        info = (runtime / relative).lstat()
        if (not stat.S_ISREG(info.st_mode) or info.st_uid != 0 or info.st_mode & 0o022):
            raise CredentialStateError('missing or unsafe selected runtime file')
    # The complete published pair must already exist. Checking cannot initialize it.
    prepare_credential(runtime_root=runtime)
    expected = [{'username': USERNAME, 'unix_socket': SOCKETS[name],
                 'tcp_host': '127.0.0.1', 'tcp_port': 0} for name in selected]
    if json.loads((runtime / 'local.json').read_text()) != expected:
        raise CredentialStateError('runtime policy differs from activation setting')
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
    except (CredentialStateError, OSError, ValueError):
        print('redis-auth-startup: invalid activation or runtime state', file=sys.stderr)
        return 1
    print('redis-auth-startup: ' + (','.join(selected) if selected else 'legacy'))
    return 0


if __name__ == '__main__':
    sys.exit(main())
