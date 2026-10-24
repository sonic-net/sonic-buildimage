"""Writer login options for explicitly selected local Redis Unix sockets."""

import json
import os
import stat


AUTH_DIRECTORY = '/run/redis-auth'


def _open(name, directory=None, private=False, is_directory=False):
    flags = os.O_RDONLY | os.O_CLOEXEC | os.O_NOFOLLOW | os.O_NONBLOCK
    if is_directory:
        flags |= os.O_DIRECTORY
    fd = os.open(name, flags, dir_fd=directory)
    info = os.fstat(fd)
    valid_type = stat.S_ISDIR(info.st_mode) if is_directory else stat.S_ISREG(info.st_mode)
    if (info.st_uid != 0 or info.st_mode & 0o022 or not valid_type
            or (private and stat.S_IMODE(info.st_mode) not in (0o400, 0o600, 0o440, 0o640))):
        os.close(fd)
        raise RuntimeError('Redis authentication file permissions are invalid')
    return fd


def _read(fd, limit):
    with os.fdopen(fd, 'rb') as source:
        value = source.read(limit + 1)
    if not value or len(value) > limit:
        raise RuntimeError('Redis authentication file size is invalid')
    return value


def _policies(value):
    entries = value if isinstance(value, list) else [value]
    if not entries:
        raise RuntimeError('Redis authentication policy is invalid')
    sockets, endpoints = set(), set()
    for entry in entries:
        if not isinstance(entry, dict) or set(entry) != {
                'username', 'unix_socket', 'tcp_host', 'tcp_port'}:
            raise RuntimeError('Redis authentication policy is invalid')
        user, path = entry['username'], entry['unix_socket']
        host, port = entry['tcp_host'], entry['tcp_port']
        if (not isinstance(user, str) or not user or len(user) > 128 or '\0' in user
                or not isinstance(path, str) or not path.startswith('/') or '\0' in path
                or host not in ('127.0.0.1', '::1') or type(port) is not int
                or not 0 <= port <= 65535 or path in sockets
                or (port > 0 and (host, port) in endpoints)):
            raise RuntimeError('Redis authentication policy is invalid')
        sockets.add(path)
        if port > 0:
            endpoints.add((host, port))
    return entries


def local_policies():
    """Return validated public endpoint settings without opening a password."""
    try:
        directory = _open(AUTH_DIRECTORY, is_directory=True)
    except FileNotFoundError:
        return []
    try:
        try:
            policy_fd = _open('local.json', directory)
        except FileNotFoundError:
            return []
        try:
            return _policies(json.loads(_read(policy_fd, 4096)))
        except (ValueError, TypeError):
            raise RuntimeError('Redis authentication policy is invalid') from None
    finally:
        os.close(directory)


def connection_kwargs(unix_socket_path=None, host='localhost', port=6379):
    """Return named-user options for a protected local Unix socket.

    An absent policy keeps existing behavior. A caller that cannot read the
    protected password keeps passwordless read access. This first adapter
    supports Unix sockets; a selected TCP endpoint must use a TLS-aware adapter.
    """
    try:
        directory = _open(AUTH_DIRECTORY, is_directory=True)
    except FileNotFoundError:
        return {}
    try:
        try:
            policy_fd = _open('local.json', directory)
        except FileNotFoundError:
            return {}
        try:
            entries = _policies(json.loads(_read(policy_fd, 4096)))
        except (ValueError, TypeError):
            raise RuntimeError('Redis authentication policy is invalid') from None
        matches = [entry for entry in entries if (
            entry['unix_socket'] == unix_socket_path if unix_socket_path else
            entry['tcp_port'] > 0 and entry['tcp_host'] == host and entry['tcp_port'] == port)]
        if not matches:
            return {}
        if not unix_socket_path:
            raise RuntimeError('Selected Redis TCP endpoint requires a TLS-aware client')
        secrets = _open('local-system', directory, is_directory=True)
        try:
            try:
                secret_fd = _open('writer.secret', secrets, private=True)
            except PermissionError:
                if os.geteuid() != 0:
                    return {}
                raise
            secret = _read(secret_fd, 128).decode('ascii')
        finally:
            os.close(secrets)
        return {'username': matches[0]['username'], 'password': secret}
    finally:
        os.close(directory)
