#!/usr/bin/env python3
"""Validate selected local process endpoints and apply Unix-only database policy."""

import json
import os
from pathlib import Path
import stat
import sys

RUNTIME = Path('/run/redis-auth')


def configure(database, additional, policy, inventory, *, environ=os.environ,
              local_ip='127.0.0.1', chassis=Path('/usr/share/sonic/platform/chassisdb.conf').exists()):
    number = environ.get('NAMESPACE_ID', '')
    count = int(environ.get('NAMESPACE_COUNT') or '1')
    if (environ.get('DATABASE_TYPE') or int(environ.get('NUM_DPU') or '0') > 0 or chassis
            or count < 1 or (number and (not number.isdecimal() or str(int(number)) != number
                                        or count == 1 or int(number) >= count))):
        raise ValueError('unsupported Redis namespace or remote topology')
    if not isinstance(policy, list) or not policy:
        raise ValueError('missing selected Redis process policy')
    if not isinstance(inventory, dict):
        raise ValueError('missing prepared Redis inventory')
    namespaces = {identity.rsplit('/', 1)[0] for identity in inventory if '/' in identity}
    if namespaces != ({'asic' + str(index) for index in range(count)} if count > 1 else set()):
        raise ValueError('prepared Redis namespaces differ from the configured ASIC count')
    prefix = 'asic' + number + '/' if number else ''
    local = {identity.removeprefix(prefix): record for identity, record in inventory.items()
             if (identity.startswith(prefix) if prefix else '/' not in identity)}
    if set(local) != set(database['INSTANCES']):
        raise ValueError('prepared Redis inventory differs from final server configuration')
    expected = {}
    protected = sum(record['protected'] is True for record in inventory.values())
    for record in inventory.values():
        if (set(record) != {'hostname', 'port', 'unix_socket_path', 'protected'}
                or record['hostname'] != '127.0.0.1' or type(record['protected']) is not bool):
            raise ValueError('invalid prepared Redis inventory')
        if record['protected']:
            expected[record['unix_socket_path']] = {
                'username': 'sonic-trusted-writer', 'unix_socket': record['unix_socket_path'],
                'tcp_host': '127.0.0.1', 'tcp_port': 0}
    if (len(expected) != protected or len(policy) != len(expected)
            or {entry['unix_socket']: entry for entry in policy} != expected):
        raise ValueError('selected Redis policy is incomplete, duplicated or differs from inventory')
    selected = set()
    for name, record in local.items():
        instance = database['INSTANCES'][name]
        extras = additional['INSTANCES'].setdefault(name, {})
        # Namespace templates use their own bridge IP, which can change when the
        # container is recreated. The entrypoint supplies its current local IP.
        if (instance['hostname'] not in ({'127.0.0.1', local_ip} if number else {'127.0.0.1'})
                or any(instance[field] != record[field] for field in ('port', 'unix_socket_path'))
                or instance.get('database_type') or extras.get('bmc_link_ip')
                or (not number and not extras.get('is_protected_mode', False))):
            raise ValueError('configured Redis endpoint differs from prepared local inventory')
        if record['protected']:
            instance['port'] = 0
            extras.update({'port': 0, 'redis_auth_enforce': True, 'is_protected_mode': True})
            selected.add(name)
    return selected


def main():
    try:
        policy_path = RUNTIME / 'local.json'
        info = policy_path.lstat()
        if (not stat.S_ISREG(info.st_mode) or info.st_uid != 0
                or info.st_mode & 0o022 or not (RUNTIME / 'server/users.acl').is_file()):
            raise ValueError('missing or unsafe selected Redis runtime policy')
        path = Path(sys.argv[1])
        database = json.loads(path.read_text())
        additional = json.loads(sys.argv[2])
        inventory_path = RUNTIME / 'server/instances.json'
        info = inventory_path.lstat()
        if not stat.S_ISREG(info.st_mode) or info.st_uid != 0 or info.st_mode & 0o022:
            raise ValueError('missing or unsafe Redis server inventory')
        selected = configure(database, additional, json.loads(policy_path.read_text()),
                             json.loads(inventory_path.read_text()), local_ip=sys.argv[3])
        # Redis creates protected sockets with mode 0766. A default directory ACL
        # can still deny a named reader; do not alter a directory shared with an
        # exempt server to compensate for that unsupported configuration.
        directories = {Path(database['INSTANCES'][name]['unix_socket_path']).parent for name in selected}
        for directory in sorted(directories):
            if 'system.posix_acl_default' in os.listxattr(directory):
                print('redis-auth-configure: protected socket directory has a default ACL: '
                      + str(directory), file=sys.stderr)
                return 1
        # This path is the entrypoint's unpublished temporary database map.
        path.write_text(json.dumps(database, indent=2) + '\n')
        print(json.dumps(additional))
        return 0
    except (OSError, ValueError, KeyError, TypeError, IndexError):
        print('redis-auth-configure: invalid local topology or runtime state', file=sys.stderr)
        return 1


if __name__ == '__main__':
    sys.exit(main())
