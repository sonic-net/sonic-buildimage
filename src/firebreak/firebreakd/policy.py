"""Validate host-owned profiles and compile explicit Redis permissions."""
from __future__ import annotations

import hashlib
import ipaddress
import json
import re

FIELDS = {
    'container', 'redis_user', 'secret_file', 'ctl_script', 'instances',
    'omit_sockets', 'use_proxy', 'acl_strict_patterns', 'acl_read_all',
    'egress_allow_ipv4', 'egress_approved_ipv4', 'egress_from_config_db',
    'egress_deny_default', 'required_processes', 'redis_tcp_proxy_port',
}
INSTANCE_FIELDS = {
    'unix_socket', 'tcp_port', 'select', 'commands', 'key_patterns',
    'read_key_patterns', 'write_key_patterns', 'channel_patterns',
}
HOST_COMMANDS = {
    'acl', 'config', 'flushall', 'flushdb', 'swapdb', 'shutdown', 'module',
    'eval', 'evalsha', 'script', 'function', 'fcall', 'fcall_ro',
    'replicaof', 'slaveof', 'migrate', 'restore', 'debug',
}


def string_list(value, field):
    if not isinstance(value, list) or any(not isinstance(x, str) or not x or
            any(c.isspace() or ord(c) < 32 for c in x) for x in value):
        raise ValueError(field + ' must be an array of nonempty strings without whitespace')
    return list(dict.fromkeys(value))


def validate_profile(cfg):
    if not isinstance(cfg, dict):
        raise ValueError('profile must be an object')
    unknown = set(cfg) - FIELDS
    if unknown:
        raise ValueError('unknown profile fields: ' + ', '.join(sorted(unknown)))
    for name in ('container', 'redis_user'):
        if not isinstance(cfg.get(name), str) or not re.fullmatch(r'[a-zA-Z0-9_-]+', cfg[name]):
            raise ValueError(name + ' must be a simple name')
    for name in ('secret_file', 'ctl_script'):
        value = cfg.get(name)
        if name == 'ctl_script' and value is None:
            continue
        if not isinstance(value, str) or not value.startswith('/') or '..' in value.split('/') or any(c.isspace() for c in value):
            raise ValueError(name + ' must be an absolute path without whitespace or traversal')
    for field in ('use_proxy', 'acl_read_all', 'acl_strict_patterns', 'egress_from_config_db', 'egress_deny_default'):
        if field in cfg and not isinstance(cfg[field], bool):
            raise ValueError(field + ' must be boolean')
    if 'redis_tcp_proxy_port' in cfg:
        port = cfg['redis_tcp_proxy_port']
        if not isinstance(port, int) or isinstance(port, bool):
            raise ValueError('redis_tcp_proxy_port must be an integer')
        if not (1024 <= port <= 65535):
            raise ValueError('redis_tcp_proxy_port must be in 1024-65535')
        instances = cfg.get('instances', {})
        if len(instances) > 1:
            raise ValueError(
                'redis_tcp_proxy_port cannot be used with multiple instances '
                '(%d); each proxy process would bind the same port' % len(instances)
            )
    if cfg.get('acl_strict_patterns', True) is not True:
        raise ValueError('automatic ACL widening is unsupported; declare explicit patterns')
    if cfg.get('egress_deny_default', True) is not True:
        raise ValueError('egress_deny_default must be true')
    for value in string_list(cfg.get('egress_allow_ipv4', []), 'egress_allow_ipv4'):
        ipaddress.IPv4Address(value)
    for value in string_list(cfg.get('egress_approved_ipv4', []), 'egress_approved_ipv4'):
        ipaddress.IPv4Network(value, strict=True)
    for value in string_list(cfg.get('omit_sockets', []), 'omit_sockets'):
        if '/' in value or value in ('.', '..'):
            raise ValueError('omit_sockets must contain basenames')
    for process in string_list(cfg.get('required_processes', []), 'required_processes'):
        if not re.fullmatch(r'[a-zA-Z0-9_.:-]+', process):
            raise ValueError('required_processes must be supervisor process names')
    instances = cfg.get('instances')
    if not isinstance(instances, dict) or not instances:
        raise ValueError('instances must be a nonempty object')
    for name, inst in instances.items():
        if not isinstance(inst, dict) or set(inst) - INSTANCE_FIELDS:
            raise ValueError('invalid or unknown instance fields: ' + str(name))
        sock = inst.get('unix_socket')
        if not isinstance(sock, str) or not re.fullmatch(r'/var/run/redis/[a-zA-Z0-9_.-]+\.sock', sock):
            raise ValueError('instance unix_socket must be a SONiC Redis socket')
        if 'tcp_port' in inst and (type(inst['tcp_port']) is not int or not 1 <= inst['tcp_port'] <= 65535):
            raise ValueError('tcp_port must be an integer port')
        if 'select' in inst and (not isinstance(inst['select'], list) or any(type(n) is not int or n < 0 for n in inst['select'])):
            raise ValueError('select must list nonnegative database indices')
        commands = string_list(inst.get('commands'), 'commands')
        for command in commands:
            if not re.fullmatch(r'[a-z][a-z0-9_-]*(\|[a-z][a-z0-9_-]*)?', command):
                raise ValueError('commands must be explicit lowercase Redis commands/subcommands')
            if command.split('|')[0] in HOST_COMMANDS and command not in ('config|get', 'acl|whoami'):
                raise ValueError('host-only command: ' + command)
            if command == 'client':
                raise ValueError('declare CLIENT subcommands explicitly')
        for field in ('key_patterns', 'read_key_patterns', 'write_key_patterns', 'channel_patterns'):
            if field in inst:
                string_list(inst[field], field)
        if 'key_patterns' in inst and 'write_key_patterns' in inst:
            raise ValueError('use write_key_patterns or legacy key_patterns, not both')
        if 'read_key_patterns' not in inst and not cfg.get('acl_read_all', False) and 'key_patterns' not in inst:
            raise ValueError('declare read_key_patterns explicitly')
    return cfg


def acl_rules(cfg, inst):
    """No implicit read-all, command categories, prefix expansion or channels.

    Legacy key_patterns remain explicit RW. SELECT metadata is not database
    authorization; Redis key permissions apply across logical databases.
    """
    read = inst.get('read_key_patterns', ['*'] if cfg.get('acl_read_all', False) else inst.get('key_patterns', []))
    write = inst.get('write_key_patterns', inst.get('key_patterns', []))
    return (['-@all'] + ['+' + c for c in inst['commands']] +
            ['%R~' + p for p in read] + ['%W~' + p for p in write] +
            ['resetchannels'] + ['&' + p for p in inst.get('channel_patterns', [])])


def approved_collectors(cfg, addresses):
    networks = [ipaddress.IPv4Network(p) for p in cfg.get('egress_approved_ipv4', [])]
    approved, rejected = [], []
    for address in addresses:
        target = approved if any(ipaddress.IPv4Address(address) in n for n in networks) else rejected
        if address not in target:
            target.append(address)
    return approved, rejected


def fingerprint(value):
    return hashlib.sha256(json.dumps(value, sort_keys=True, separators=(',', ':')).encode()).hexdigest()
