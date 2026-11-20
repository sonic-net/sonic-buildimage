#!/usr/bin/env python3
"""Prepare one Redis writer password for the current host boot."""

import fcntl
import hashlib
import os
import secrets
import shutil
import stat
import tempfile


DEFAULT_RUNTIME_ROOT = "/run/redis-auth"
INSTANCE = "local-system"
_NOFOLLOW = os.O_NOFOLLOW | os.O_CLOEXEC


class CredentialStateError(RuntimeError):
    """Runtime credential state is unsafe or incomplete."""


def _check(info, mode, *, directory=False):
    correct_type = stat.S_ISDIR(info.st_mode) if directory else stat.S_ISREG(info.st_mode)
    if (not correct_type or stat.S_IMODE(info.st_mode) != mode
            or info.st_uid != 0 or info.st_gid != 0
            or (not directory and info.st_nlink != 1)):
        raise CredentialStateError("incorrect file type, ownership, mode, or link count")


def _runtime_directory(path, *, create=True):
    # Production uses /run/redis-auth. The override is only for isolated tests.
    if (not os.path.isabs(path) or path.startswith("//")
            or os.path.normpath(path) != path or path == "/"):
        raise CredentialStateError("runtime root must be an absolute directory")
    ancestor = os.path.dirname(path)
    while ancestor != "/":
        info = os.lstat(ancestor)
        sticky_tmp = ancestor == "/tmp" and info.st_mode & stat.S_ISVTX
        if (not stat.S_ISDIR(info.st_mode) or info.st_uid != 0
                or (info.st_mode & 0o022 and not sticky_tmp)):
            raise CredentialStateError("unsafe runtime directory ancestor")
        ancestor = os.path.dirname(ancestor)
    if create:
        try:
            os.mkdir(path, 0o755)
        except FileExistsError:
            pass
        else:
            os.chown(path, 0, 0, follow_symlinks=False)
            # Readers need directory search permission; credential files remain 0400.
            # nosemgrep: python.lang.security.audit.insecure-file-permissions.insecure-file-permissions
            os.chmod(path, 0o755, follow_symlinks=False)
    _check(os.lstat(path), 0o755, directory=True)


def _read(path, size):
    try:
        fd = os.open(path, os.O_RDONLY | os.O_NONBLOCK | _NOFOLLOW)
    except OSError:
        raise CredentialStateError("runtime credential pair is incomplete or unsafe") from None
    with os.fdopen(fd, "rb") as stream:
        info = os.fstat(stream.fileno())
        _check(info, 0o400)
        if info.st_size != size:
            raise CredentialStateError("invalid runtime credential size")
        content = stream.read(size + 1)
    if len(content) != size:
        raise CredentialStateError("runtime credential changed while reading")
    return content


def _write(path, content):
    fd = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_EXCL | _NOFOLLOW, 0o400)
    with os.fdopen(fd, "wb") as stream:
        os.fchown(stream.fileno(), 0, 0)
        os.fchmod(stream.fileno(), 0o400)
        stream.write(content)


def prepare_credential(*, runtime_root=DEFAULT_RUNTIME_ROOT, create=True):
    """Return the validated digest, creating the boot pair only when permitted."""
    if os.geteuid() != 0:
        raise CredentialStateError("must run as root")
    runtime_root = os.fspath(runtime_root)
    _runtime_directory(runtime_root, create=create)
    lock_fd = os.open(runtime_root, os.O_RDONLY | os.O_DIRECTORY | _NOFOLLOW)
    try:
        fcntl.flock(lock_fd, fcntl.LOCK_EX)
        target = os.path.join(runtime_root, INSTANCE)
        if os.path.lexists(target):
            _check(os.lstat(target), 0o755, directory=True)
            password = _read(os.path.join(target, "writer.secret"), 43)
            digest = _read(os.path.join(target, "writer.sha256"), 65)
            if hashlib.sha256(password).hexdigest().encode("ascii") + b"\n" != digest:
                raise CredentialStateError("runtime credential pair does not match")
            return digest.decode("ascii").strip()

        if not create or any(os.path.lexists(os.path.join(runtime_root, name))
               for name in ("local.json", "server/users.acl", "server/instances.json")):
            raise CredentialStateError("published boot credential is missing; reboot required")

        temporary = tempfile.mkdtemp(prefix=".local-system-", dir=runtime_root)
        try:
            password = secrets.token_urlsafe(32).encode("ascii")
            digest = hashlib.sha256(password).hexdigest().encode("ascii") + b"\n"
            _write(os.path.join(temporary, "writer.secret"), password)
            _write(os.path.join(temporary, "writer.sha256"), digest)
            os.chown(temporary, 0, 0, follow_symlinks=False)
            # Readers need directory search permission; credential files remain 0400.
            # nosemgrep: python.lang.security.audit.insecure-file-permissions.insecure-file-permissions
            os.chmod(temporary, 0o755, follow_symlinks=False)
            os.rename(temporary, target)
            temporary = None
        finally:
            if temporary is not None:
                shutil.rmtree(temporary)
        return digest.decode("ascii").strip()
    finally:
        os.close(lock_fd)
