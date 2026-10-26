#!/usr/bin/env python3
"""Start an enforced Redis process and grant its socket to existing readers."""

import argparse
import json
import os
from pathlib import Path
import signal
import socket
import stat
import struct
import subprocess
import sys
import time


GROUPS = Path('/run/redis-auth/server/socket-groups.json')


def reader_acl(path=GROUPS):
    fd = os.open(path, os.O_RDONLY | os.O_NONBLOCK | os.O_NOFOLLOW | os.O_CLOEXEC)
    with os.fdopen(fd) as stream:
        info = os.fstat(stream.fileno())
        if (not stat.S_ISREG(info.st_mode) or info.st_uid != 0
                or info.st_mode & 0o022):
            raise RuntimeError('unsafe socket reader group policy')
        groups = json.load(stream)
    if (not isinstance(groups, list) or not groups or len(groups) > 3
            or any(type(gid) is not int or not 0 < gid < 2**32 - 1 for gid in groups)):
        raise RuntimeError('invalid socket reader group policy')
    entries = ['user::rwx', 'group::rw-']
    entries.extend(f'group:{gid}:rw-' for gid in sorted(set(groups)))
    return ','.join(entries + ['mask::rw-', 'other::---'])


def child_socket(path, process):
    """Return the socket identity once this child owns the listening endpoint."""
    try:
        info = path.lstat()
    except FileNotFoundError:
        return None
    if not stat.S_ISSOCK(info.st_mode) or info.st_uid != os.geteuid():
        raise RuntimeError('selected endpoint is not a socket owned by Redis')
    with socket.socket(socket.AF_UNIX) as connection:
        connection.settimeout(0.1)
        try:
            connection.connect(str(path))
        except (ConnectionRefusedError, FileNotFoundError, socket.timeout):
            return None
        peer_pid, _, _ = struct.unpack('3i', connection.getsockopt(
            socket.SOL_SOCKET, socket.SO_PEERCRED, struct.calcsize('3i')))
        if peer_pid != process.pid:
            raise RuntimeError('selected endpoint belongs to another process')
    return info.st_dev, info.st_ino


def run(path, command, *, timeout=15, groups_path=GROUPS):
    acl = reader_acl(groups_path)
    process = None
    interrupted = None

    def forward(signum, _frame):
        nonlocal interrupted
        interrupted = signum
        if process is not None and process.poll() is None:
            try:
                process.send_signal(signum)
            except ProcessLookupError:
                pass

    previous = {signum: signal.signal(signum, forward)
                for signum in (signal.SIGTERM, signal.SIGINT)}
    try:
        process = subprocess.Popen(command)
        if interrupted is not None:
            forward(interrupted, None)
        deadline = time.monotonic() + timeout
        while process.poll() is None:
            if interrupted is not None:
                return process.wait()
            identity = child_socket(path, process)
            if identity is not None:
                # Replace inherited named entries too: only the owning group and
                # the host's Redis, users and sudo groups receive socket access.
                subprocess.run(['/usr/bin/setfacl', '--set', acl, str(path)],
                               check=True, timeout=5)
                info = path.lstat()
                if ((info.st_dev, info.st_ino) != identity
                        or stat.S_IMODE(info.st_mode) != 0o760):
                    raise RuntimeError('selected socket changed while applying reader access')
                print('redis-auth-server: reader socket access ready', flush=True)
                return process.wait()
            if time.monotonic() >= deadline:
                raise RuntimeError('timed out waiting for the selected Redis socket')
            time.sleep(0.02)
        if process.returncode:
            return process.returncode
        raise RuntimeError('Redis exited before reader socket access was ready')
    finally:
        if process is not None and process.poll() is None:
            process.terminate()
            try:
                process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait()
        for signum, handler in previous.items():
            signal.signal(signum, handler)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--groups', type=Path, default=GROUPS)
    parser.add_argument('socket', type=Path)
    parser.add_argument('command', nargs=argparse.REMAINDER)
    args = parser.parse_args()
    if not args.command:
        parser.error('a Redis command is required')
    try:
        result = run(args.socket, args.command, groups_path=args.groups)
        return result if result >= 0 else 128 - result
    except (OSError, ValueError, RuntimeError, subprocess.SubprocessError) as error:
        print(f'redis-auth-server: {error}', file=sys.stderr)
        return 1


if __name__ == '__main__':
    sys.exit(main())
