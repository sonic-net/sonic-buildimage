#!/usr/bin/env python3
"""Validate selected local process endpoints and apply Unix-only database policy."""

import json
import os
from pathlib import Path
import stat
import sys

RUNTIME = Path('/run/redis-auth')
SOCKETS = {'redis': '/var/run/redis/redis.sock',
           'redis_bmp': '/var/run/redis/redis_bmp.sock'}


def configure(database, additional, policy, *, environ=os.environ,
              chassis=Path('/usr/share/sonic/platform/chassisdb.conf').exists()):
    if (environ.get('NAMESPACE_ID') or environ.get('DATABASE_TYPE')
            or int(environ.get('NAMESPACE_COUNT') or '1') > 1
            or int(environ.get('NUM_DPU') or '0') > 0 or chassis):
        raise ValueError('selected Redis protection supports local single-host processes only')
    if isinstance(policy, dict):
        policy = [policy]
    if not isinstance(policy, list) or not policy:
        raise ValueError('missing selected Redis process policy')
    selected = set()
    for entry in policy:
        if (set(entry) != {'username', 'unix_socket', 'tcp_host', 'tcp_port'}
                or entry['username'] != 'sonic-trusted-writer'
                or entry['tcp_host'] != '127.0.0.1' or entry['tcp_port'] != 0):
            raise ValueError('unsupported selected Redis process policy')
        names = [name for name, socket in SOCKETS.items() if socket == entry['unix_socket']]
        if len(names) != 1 or names[0] in selected:
            raise ValueError('ambiguous selected Redis process')
        name = names[0]
        instance = database['INSTANCES'][name]
        extras = additional['INSTANCES'].setdefault(name, {})
        if (instance['hostname'] != '127.0.0.1'
                or instance['unix_socket_path'] != SOCKETS[name]
                or instance.get('database_type') or extras.get('bmc_link_ip')
                or not extras.get('is_protected_mode', False)):
            raise ValueError('selected Redis process has a nonlocal or unexpected endpoint')
        instance['port'] = 0
        extras.update({'port': 0, 'redis_auth_enforce': True})
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
        configure(database, additional, json.loads(policy_path.read_text()))
        # This path is the entrypoint's unpublished temporary database map.
        path.write_text(json.dumps(database, indent=2) + '\n')
        print(json.dumps(additional))
        return 0
    except (OSError, ValueError, KeyError, TypeError, IndexError):
        print('redis-auth-configure: invalid local topology or runtime state', file=sys.stderr)
        return 1


if __name__ == '__main__':
    sys.exit(main())
