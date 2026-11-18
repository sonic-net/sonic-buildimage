"""Declarative newc writer with GNU cpio --reproducible hardlink semantics.

Only reads the staged tree; no target executables, cpio or shell tools are run.
metadata(path, stat) supplies the logical mode/mtime of unprivileged staging.
Owners and device numbers are zero, inode numbers are allocated in path order,
and payloads of hardlinked regular files are deferred until the last link.
"""
import collections
import hashlib
import os
import stat


_NEWC_FIELDS = ('ino', 'mode', 'uid', 'gid', 'nlink', 'mtime', 'filesize',
                'devmajor', 'devminor', 'rdevmajor', 'rdevminor', 'namesize', 'check')


def _header_value(name, value):
    if isinstance(value, bool) or not isinstance(value, int) or not 0 <= value <= 0xffffffff:
        raise ValueError('newc ' + name + ' must be an unsigned 32-bit integer')
    return value


def write_newc(root, output, metadata=None):
    root = os.fspath(root)
    entries = []

    def walk(rel):
        directory = os.path.join(root, rel)
        for name in os.listdir(directory):
            path = rel + '/' + name if rel else name
            entries.append(path)
            if stat.S_ISDIR(os.lstat(os.path.join(root, path)).st_mode):
                walk(path)

    walk('')
    entries.sort(key=lambda path: ('./' + path).encode())
    entries.insert(0, '.')
    subdirs = collections.Counter()
    for path in entries[1:]:
        if stat.S_ISDIR(os.lstat(os.path.join(root, path)).st_mode):
            subdirs[path.rsplit('/', 1)[0] if '/' in path else '.'] += 1
    inodes = {}
    deferred = []
    listing = []
    with open(output, 'wb') as stream:
        position = 0

        def identity(st):
            return st.st_dev, st.st_ino

        def emit(name, st, mode, mtime, nlink, data):
            nonlocal position
            ino = inodes.setdefault(identity(st), len(inodes) + 1) if st is not None else 0
            encoded = name.encode() + b'\0'
            values = (ino, mode, 0, 0, nlink, mtime, len(data), 0, 0, 0, 0, len(encoded), 0)
            header = b'070701' + b''.join(b'%08X' % _header_value(field, value)
                                          for field, value in zip(_NEWC_FIELDS, values))
            blob = header + encoded
            blob += b'\0' * (-len(blob) % 4)
            blob += data + b'\0' * (-len(data) % 4)
            stream.write(blob)
            position += len(blob)

        for path in entries:
            host = os.path.join(root, path)
            st = os.lstat(host)
            mode, mtime = metadata(host, st) if metadata else (st.st_mode & ~0o7000, 0)
            mode, mtime = _header_value('mode', mode), _header_value('mtime', mtime)
            if stat.S_IFMT(mode) != stat.S_IFMT(st.st_mode):
                raise ValueError('metadata changes file type: ' + path)
            if stat.S_ISDIR(st.st_mode):
                data, nlink = b'', 2 + subdirs[path]
            elif stat.S_ISLNK(st.st_mode):
                data, nlink = os.readlink(host).encode(), 1
            elif stat.S_ISREG(st.st_mode):
                with open(host, 'rb') as source:
                    data = source.read()
                nlink = st.st_nlink
            else:
                raise ValueError('unexpected file type in initramfs: ' + path)
            listing.append(dict(name=path, mode='%o' % mode, mtime=mtime,
                                size=len(data), nlink=nlink,
                                link=data.decode() if stat.S_ISLNK(mode) else None,
                                sha256=hashlib.sha256(data).hexdigest() if stat.S_ISREG(mode) else None))
            if stat.S_ISREG(st.st_mode) and nlink > 1:
                same = [entry for entry in deferred if identity(entry[1]) == identity(st)]
                if len(same) + 1 < nlink:
                    deferred.insert(0, (path, st, mode, mtime, nlink))
                    continue
                for entry in same:
                    emit(*entry, b'')
                deferred = [entry for entry in deferred if identity(entry[1]) != identity(st)]
            emit(path, st, mode, mtime, nlink, data)
        if deferred:
            raise ValueError('incomplete hard-link groups: %r' % [entry[0] for entry in deferred])
        emit('TRAILER!!!', None, 0, 0, 1, b'')
        stream.write(b'\0' * (-position % 512))
    return listing
