"""Replay SONiC initramfs-tools 0.142 using explicit rootfs and execution tools.

Adapted from the reference buildimage platform/common/rootfs/initrd replayer.
This project retains its patched 0.142 source on Trixie. Package hooks are
reviewed by script digest; unknown scripts fail rather than silently using
another distribution's behavior. Boot and kdump receive their corresponding
rootfs state and config_dir. Kernel release/architecture come from the kernel
producer's image metadata. Native modprobe/depmod/zstd are declared tools;
compressed target modules are inspected and decompressed as 0.142 specifies.
No target executable or host chroot is executed.
"""

import collections
import fnmatch
import gzip
import hashlib
import io
import lzma
import json
import os
import re
import shutil
import stat
import struct
import subprocess
import sys
import tarfile
import tempfile

from ldcache import generate_cache
from newc import write_newc
from kernel_inputs import read_kernel_metadata as read_boot_metadata

PINS = {
    'usr/sbin/mkinitramfs': '5a75ff72dc960544213c5fb95a22f18c23be75fcc5bdf1803ade278162d9ddb9',
    'usr/share/initramfs-tools/hook-functions': '9ec3461ea56b387f777ac1448f14c89ede2dbe21c3eae24e99ae4ccbde1c591d',
    'usr/share/initramfs-tools/conf-hooks.d/busybox': '0d86ad66470b73e020ca349e66262c6cbab1f2f1d0c88836ca28f8e4b8e6828b',
}
# hook name -> sha256 of the replayed script (package hooks and SONiC
# files/initramfs-tools hooks).
HOOK_PINS = {
    'dmsetup': 'd7aba5b55840b07c8b885d7fcd18c6e5aa9b9ed0c266968e06d56d3dde2743cd',
    'fsck': 'a33261a85749be4f2cf10e9c8aa2696197da41f94df8c5f108df74defd134fd4',
    'kdump-tools': '2c137f46748bfea1df7f776e44d1cd2f217145b69f439ca204545a37aff2484d',
    'keymap': '34f8113dd5c751af052b1c6fe71abdb64629b624990449a77917d7b4ae0793db',
    'klibc-utils': '56e06e437c8b05f066d3f86f9af7b49425b27949164aaf83890dbe154ebe42ba',
    'kmod': '23eeee72b630029cc717ccc4c4e927fa35f55e3c30e66c1dd85713fe2100c7f1',
    'resume': '71182a2cba9c6964511752f2f6a5758869c30f4b1caa4888de3c9c26bd318e6a',
    'thermal': '0e8ec63b8c3b745944b90e71bec7ef026d38ce423fc04b0fa9a176db22c5d49e',
    'udev': 'a1aab3a02b3ec52fca3f6287f3b7185cf3d49756ca7df6de4fcd5d2a04a49a86',
    'zz-busybox': '41b934003c3683a39d0aba0fee8fc4febffabce90498d2e126c1648b55ec64ae',
    'mke2fs': '32263b1b9ee1a03b6c83bc4343c859d512a95eb38ced7e118157f6d9a13a5be0',
    'setfacl': '858f6bff2d151b8cbc63c2f1e0c0eed4037fd9da457da0c2fa3cb31f3c76fa5a',
    'union-fsck': '79060b169312934aa80180a059573910c393243a9d9cc41df01f616e6d8b9c2b',
    'pzstd': 'a67afa491c60b70252fb31bbd873b4d648401274745b3b7d7ad1467cf9f385bf',
    'file': 'f4ccde963908b7fa8d4422a2f567736bcf1e45fc9d6423eb11a9e459f3c20810',
    'uboot-utils': '1670d2f7d40027340a536c874c238f1682090ce269f49ce71d9f3e1bfc7ecacc',
}
ARCH = {
    'amd64': dict(machine=62, triplet='x86_64-linux-gnu', rtld='/lib64/ld-linux-x86-64.so.2', cache_flags=0x303),
    'arm64': dict(machine=183, triplet='aarch64-linux-gnu', rtld='/lib/ld-linux-aarch64.so.1', cache_flags=0xa03),
}
CONF_NAME = re.compile(r'^[A-Za-z0-9][A-Za-z0-9._-]*$')
SCRIPT_NAME = re.compile(r'^[A-Za-z0-9._-]+$')
USR_MERGED = re.compile(r'^/(bin/|lib|sbin/)')
ENV = {'LC_ALL': 'C', 'TZ': 'UTC', 'PATH': '/nonexistent'}


def sha256(path):
    h = hashlib.sha256()
    with open(path, 'rb') as f:
        for chunk in iter(lambda: f.read(1 << 20), b''):
            h.update(chunk)
    return h.hexdigest()


# ---------------------------------------------------------------------------
# Rootfs: ordered layer tars with OCI whiteouts, metadata kept on the side
# (the action runs unprivileged: owners and special bits cannot be applied).

class Rootfs:
    def __init__(self, host):
        self.h = host
        self.meta = {}  # canonical rel path -> (st_mode, mtime, uid, gid)

    # -- in-root path resolution (absolute symlinks are relative to root) --
    def canon(self, path, follow=True, missing='none'):
        """Canonical chroot path. missing: 'none' (all must exist, else
        None), 'last' (readlink -f), 'all' (realpath -m)."""
        parts = collections.deque(p for p in path.split('/') if p and p != '.')
        cur = []
        hops = 0
        while parts:
            c = parts.popleft()
            if c == '..':
                if cur:
                    cur.pop()
                continue
            cand = cur + [c]
            try:
                st = os.lstat(self.h + '/' + '/'.join(cand))
            except (FileNotFoundError, NotADirectoryError):
                if missing == 'all' or (missing == 'last' and not parts):
                    cur = cand
                    continue
                return None
            if stat.S_ISLNK(st.st_mode) and (parts or follow):
                hops += 1
                if hops > 40:
                    return None
                target = os.readlink(self.h + '/' + '/'.join(cand))
                if target.startswith('/'):
                    cur = []
                parts.extendleft(reversed([p for p in target.split('/') if p and p != '.']))
                continue
            if parts and not stat.S_ISDIR(st.st_mode):
                if missing == 'all':
                    cur = cand
                    continue
                return None
            cur = cand
        return '/' + '/'.join(cur)

    def host(self, cpath):
        return self.h + cpath if cpath != '/' else self.h

    def lstat(self, path, follow):
        c = self.canon(path, follow=follow)
        if c is None:
            return None
        try:
            return os.lstat(self.host(c))
        except FileNotFoundError:
            return None

    def exists(self, p):
        return self.lstat(p, True) is not None

    def isfile(self, p):
        st = self.lstat(p, True)
        return st is not None and stat.S_ISREG(st.st_mode)

    def isdir(self, p):
        st = self.lstat(p, True)
        return st is not None and stat.S_ISDIR(st.st_mode)

    def islink(self, p):
        st = self.lstat(p, False)
        return st is not None and stat.S_ISLNK(st.st_mode)

    def lexists(self, p):
        return self.lstat(p, False) is not None

    def readlink_f(self, p):
        return self.canon(p, missing='last')

    def rmeta(self, cpath):
        st = os.lstat(self.host(cpath))
        m = self.meta.get(cpath.lstrip('/'))
        if m is None or stat.S_IFMT(m[0]) != stat.S_IFMT(st.st_mode):
            return (stat.S_IFMT(st.st_mode) | (0o755 if stat.S_ISDIR(st.st_mode) else 0o644), 0, 0, 0)
        return m

    def is_exec(self, p):
        c = self.canon(p)
        if c is None or not stat.S_ISREG(os.lstat(self.host(c)).st_mode):
            return False
        return bool(self.rmeta(c)[0] & 0o111)

    def glob(self, dirpath, pattern):
        """Shell glob DIR/PATTERN (no dot files), sorted like sh."""
        c = self.canon(dirpath)
        if c is None or not os.path.isdir(self.host(c)):
            return []
        names = sorted(n for n in os.listdir(self.host(c)) if not n.startswith('.') and fnmatch.fnmatchcase(n, pattern))
        return [dirpath.rstrip('/') + '/' + n for n in names]

    def read(self, p):
        with open(self.host(self.canon(p)), 'rb') as f:
            return f.read()

    # -- materialization --
    def _remove(self, hp):
        if os.path.isdir(hp) and not os.path.islink(hp):
            shutil.rmtree(hp)
        else:
            os.unlink(hp)

    def _drop_meta(self, rel):
        self.meta.pop(rel, None)
        prefix = rel + '/'
        for k in [k for k in self.meta if k.startswith(prefix)]:
            del self.meta[k]

    def _parent(self, rel):
        d, _, base = rel.rpartition('/')
        if not d:
            return '', base
        c = self.canon('/' + d, missing='all')
        cur = ''
        for comp in c.strip('/').split('/'):
            cur = cur + '/' + comp
            hp = self.h + cur
            if not os.path.lexists(hp):
                os.mkdir(hp, 0o755)
            elif not os.path.isdir(hp) or os.path.islink(hp):
                raise ValueError('layer parent is not a directory: ' + cur)
        return c.strip('/'), base

    def apply_layer(self, path, skip_prefixes=()):
        with tarfile.open(path, 'r:*') as tf:
            members = tf.getmembers()
            for m in members:
                if '..' in m.name.split('/'):
                    raise ValueError('parent component in rootfs layer member: ' + m.name)
                if m.islnk() and '..' in m.linkname.split('/'):
                    raise ValueError('parent component in rootfs hardlink target: ' + m.linkname)
                parts = [part for part in m.name.split('/') if part not in ('', '.')]
                base = parts[-1] if parts else ''
                if base.startswith('.wh.') and base[4:] in ('', '.', '..'):
                    raise ValueError('invalid rootfs whiteout target: ' + m.name)
            entries = []
            for m in members:
                name = m.name
                while name.startswith('./'):
                    name = name[2:]
                name = name.strip('/')
                if not name or name == '.':
                    continue
                if any(name == p or name.startswith(p + '/') for p in skip_prefixes):
                    continue
                entries.append((m, name))

            # OCI whiteouts remove lower-layer state, regardless of where the
            # marker appears relative to this layer's own entries.
            for m, name in entries:
                base = name.rsplit('/', 1)[-1]
                if not base.startswith('.wh.'):
                    continue
                pd = name.rsplit('/', 1)[0] if '/' in name else ''
                pc = self.canon('/' + pd) if pd else '/'
                if pc is None:
                    continue
                d = pc.strip('/')
                if base == '.wh..wh..opq':
                    hd = self.h + '/' + d if d else self.h
                    for n in os.listdir(hd):
                        r = (d + '/' + n) if d else n
                        self._remove(hd + '/' + n)
                        self._drop_meta(r)
                else:
                    r = (d + '/' + base[4:]) if d else base[4:]
                    if os.path.lexists(self.h + '/' + r):
                        self._remove(self.h + '/' + r)
                        self._drop_meta(r)

            # Preserve stream order for ordinary entries and hardlinks.
            for m, name in entries:
                if name.rsplit('/', 1)[-1].startswith('.wh.'):
                    continue
                # Debian merged-/usr package extraction keeps a directory
                # alias supplied by base-files when an older package names
                # that directory explicitly (tar --keep-directory-symlink).
                if m.isdir() and self.islink('/' + name) and self.isdir('/' + name):
                    continue
                d, base = self._parent(name)
                rel = (d + '/' + base) if d else base
                hp = self.h + '/' + rel
                if os.path.lexists(hp) and not (m.isdir() and os.path.isdir(hp) and not os.path.islink(hp)):
                    self._remove(hp)
                    self._drop_meta(rel)
                if m.isdir():
                    if not os.path.lexists(hp):
                        os.mkdir(hp, 0o755)
                    os.chmod(hp, (m.mode & 0o777) | 0o700)
                    mode = stat.S_IFDIR
                elif m.issym():
                    os.symlink(m.linkname, hp)
                    mode = stat.S_IFLNK
                elif m.islnk():
                    target = m.linkname
                    while target.startswith('./'):
                        target = target[2:]
                    td, tb = self._parent(target.strip('/'))
                    tr = (td + '/' + tb) if td else tb
                    os.link(self.h + '/' + tr, hp, follow_symlinks=False)
                    src = self.meta.get(tr)
                    self.meta[rel] = (src[0] if src else stat.S_IFREG | m.mode, m.mtime, m.uid, m.gid)
                    continue
                elif m.isreg():
                    with tf.extractfile(m) as src, open(hp, 'wb') as dst:
                        shutil.copyfileobj(src, dst, 1 << 20)
                    os.chmod(hp, (m.mode & 0o777) | 0o600)
                    mode = stat.S_IFREG
                else:
                    continue  # device nodes / fifos: never read by mkinitramfs
                self.meta[rel] = (mode | (m.mode & 0o7777), int(m.mtime), m.uid, m.gid)


# ---------------------------------------------------------------------------
# ELF helpers (ldd and modinfo replacements).

def elf_read(path, data=None):
    if data is None:
        with open(path, 'rb') as f:
            data = f.read()
    if data[:4] != b'\x7fELF':
        return None
    if len(data) < 6:
        raise ValueError('truncated ELF identification in ' + path)
    if data[4] != 2 or data[5] != 1:
        raise ValueError('unsupported ELF class or encoding in ' + path)
    if len(data) < 64:
        raise ValueError('truncated ELF header in ' + path)
    e_type, e_machine = struct.unpack_from('<HH', data, 16)
    e_phoff, e_shoff = struct.unpack_from('<QQ', data, 32)
    e_phentsize, e_phnum, e_shentsize, e_shnum, e_shstrndx = struct.unpack_from('<HHHHH', data, 54)
    phdrs = [struct.unpack_from('<IIQQQQQQ', data, e_phoff + i * e_phentsize) for i in range(e_phnum)]
    info = dict(machine=e_machine, interp=None, needed=[], rpath=None, runpath=None, dynamic=False, data=data,
                shoff=e_shoff, shentsize=e_shentsize, shnum=e_shnum, shstrndx=e_shstrndx)
    loads = [p for p in phdrs if p[0] == 1]

    def v2o(vaddr):
        for p in loads:
            if p[3] <= vaddr < p[3] + p[6]:
                return vaddr - p[3] + p[2]
        raise ValueError('unmapped address in ' + path)
    for p in phdrs:
        if p[0] == 3:
            info['interp'] = data[p[2]:p[2] + p[5]].split(b'\0')[0].decode()
        elif p[0] == 2:
            info['dynamic'] = True
            entries = []
            off = p[2]
            while off + 16 <= p[2] + p[5]:
                tag, val = struct.unpack_from('<qQ', data, off)
                off += 16
                if tag == 0:
                    break
                entries.append((tag, val))
            strtab = [v for t, v in entries if t == 5]
            if strtab:
                base = v2o(strtab[0])

                def s(o):
                    return data[base + o:data.index(b'\0', base + o)].decode()
                info['needed'] = [s(v) for t, v in entries if t == 1]
                rp = [s(v) for t, v in entries if t == 15]
                ru = [s(v) for t, v in entries if t == 29]
                info['rpath'] = rp[0] if rp else None
                info['runpath'] = ru[0] if ru else None
    return info


def elf_section(info, wanted):
    data = info['data']
    shdrs = [struct.unpack_from('<IIQQQQIIQQ', data, info['shoff'] + i * info['shentsize']) for i in range(info['shnum'])]
    if not shdrs:
        return None
    strsec = shdrs[info['shstrndx']]
    for sh in shdrs:
        nm = data[strsec[4] + sh[0]:data.index(b'\0', strsec[4] + sh[0])].decode()
        if nm == wanted:
            return data[sh[4]:sh[4] + sh[5]]
    return None


def ld_cache(raw, flags):
    if raw[:20] != b'glibc-ld.so.cache1.1':
        raise ValueError('unexpected ld.so.cache format')
    count = struct.unpack_from('<I', raw, 20)[0]
    result = {}
    for i in range(count):
        fl, key, value, _osv, hwcap = struct.unpack_from('<iIIIQ', raw, 48 + 24 * i)
        if fl != flags or hwcap:
            continue
        name = raw[key:raw.index(0, key)].decode()
        result.setdefault(name, raw[value:raw.index(0, value)].decode())
    return result


# ---------------------------------------------------------------------------

def gnu_tsort(pairs):
    """coreutils tsort: zero in-degree items scanned in strcmp order, each
    item's successors LIFO, newly freed items appended to the queue."""
    items = {}
    for a, b in pairs:
        items.setdefault(a, [0, []])
        items.setdefault(b, [0, []])
        if a != b:
            items[b][0] += 1
            items[a][1].insert(0, b)
    queue = [n for n in sorted(items, key=lambda s: s.encode()) if items[n][0] == 0]
    i = 0
    while i < len(queue):
        for s in items[queue[i]][1]:
            items[s][0] -= 1
            if items[s][0] == 0:
                queue.append(s)
        i += 1
    if len(queue) != len(items):
        raise ValueError('tsort: loop in ' + repr(pairs))
    return queue


def script_prereqs(text, path):
    """Output of `<script> prereqs` for the boot-script idioms in use:
    `prereqs) exit 0`, `prereqs) echo "..."` and `prereqs) prereqs` with
    `prereqs() { echo "$PREREQ[S]"; }` and a literal PREREQ[S]="..."."""
    m = re.search(r'^\s*prereqs\)(.*?);;', text, re.M | re.S)
    if m is None:
        raise ValueError('no prereqs case in ' + path)
    branch = [l.strip() for l in m.group(1).strip().splitlines() if l.strip() and not l.strip().startswith('#')]
    if branch == ['exit 0']:
        return ''
    if len(branch) == 2 and branch[1] == 'exit 0':
        lit = re.fullmatch(r'echo\s+"([^"$`]*)"', branch[0])
        if lit:
            return lit.group(1)
        if branch[0] == 'prereqs':
            f = re.search(r'^\s*prereqs\s*\(\)\s*\{?\s*\n?\s*\{?\s*echo\s+"\$(\w+)";?\s*\}', text, re.M)
            if f:
                a = re.findall(r'^\s*' + f.group(1) + r'="([^"$`]*)"\s*$', text, re.M)
                if len(a) == 1:
                    return a[0]
    raise ValueError('cannot determine prereqs of ' + path)


def shell_conf(text, path):
    """Assignments-only shell config (initramfs.conf, conf.d/*)."""
    out = {}
    for line in text.splitlines():
        line = line.strip()
        if not line or line.startswith('#'):
            continue
        m = re.match(r'^(?:export\s+)?([A-Za-z_][A-Za-z0-9_]*)=(.*)$', line)
        if not m:
            raise ValueError('unsupported statement in %s: %r' % (path, line))
        v = m.group(2).strip()
        if len(v) >= 2 and v[0] == v[-1] and v[0] in '"\'':
            v = v[1:-1]
        elif re.search(r'[\s$`;]', v):
            raise ValueError('unsupported value in %s: %r' % (path, line))
        out[m.group(1)] = v
    return out


class Mkinitramfs:
    def __init__(self, root, spec, work):
        self.r = root
        self.spec_modules = spec.get('modules', 'most')
        self.spec_compression = spec.get('compression', 'zstd')
        self.spec_compression_level = spec.get('compression_level', -1)
        self.arch = spec['arch']
        self.a = ARCH[self.arch]
        self.version = spec['kernel_release']
        self.modprobe = spec['modprobe']
        self.zstd = spec.get('zstd')
        self.depmod = spec['depmod']
        with open(spec['busybox_applets']) as f:
            self.busybox_manifest = json.load(f)
        self.work = work
        self.D = '/var/tmp/mkinitramfs_bazel'
        self.dmeta = {}  # host inode -> (mode, mtime, host mtime_ns)
        self.log = []
        self.hooks_run = []
        self.orders = {}
        self.cache = None

    # -- declared native exec-platform tools -----------------------------
    def run(self, cmd, **kw):
        res = subprocess.run(cmd, env=ENV, capture_output=True, **kw)
        if res.returncode != 0:
            raise RuntimeError('%s failed (%d): %s' % (cmd, res.returncode, res.stderr.decode(errors='replace')))
        return res

    # -- DESTDIR bookkeeping ---------------------------------------------
    def dpath(self, target):
        return self.D + '/' + target.lstrip('/')

    def note(self, hp, mode, mtime):
        st = os.lstat(hp)
        self.dmeta[st.st_ino] = (mode, mtime, st.st_mtime_ns)

    def mkdir_p(self, cpath):
        c = self.r.canon(cpath, missing='all')
        cur = ''
        for comp in c.strip('/').split('/'):
            cur += '/' + comp
            hp = self.r.h + cur
            if not os.path.lexists(hp):
                os.mkdir(hp, 0o755)
                self.note(hp, stat.S_IFDIR | 0o755, 0)
            elif os.path.islink(hp):
                raise ValueError('mkdir_p hit an unresolved symlink ' + cur)
        return c

    def place(self, cdest):
        """Host path for creating chroot path cdest (parent must exist)."""
        d, _, b = cdest.rstrip('/').rpartition('/')
        pc = self.r.canon(d or '/')
        if pc is None or not os.path.isdir(self.r.host(pc)):
            raise ValueError('missing parent directory for ' + cdest)
        return (pc.rstrip('/') + '/' + b), self.r.host(pc).rstrip('/') + '/' + b

    def cp_p(self, src, dest, deref=True, no_clobber=False):
        """cp -p / cp -pP / cp -a of one source to file or directory dest."""
        if self.r.isdir(dest):
            dest = dest.rstrip('/') + '/' + src.rstrip('/').rsplit('/', 1)[-1]
        csrc = self.r.canon(src, follow=deref)
        if csrc is None:
            raise ValueError('cp: missing ' + src)
        _, hdst = self.place(dest)
        if os.path.lexists(hdst):
            if no_clobber:
                return
            if os.path.isdir(hdst) and not os.path.islink(hdst):
                raise ValueError('cp onto directory ' + dest)
            os.unlink(hdst)
        hsrc = self.r.host(csrc)
        meta = self.r.rmeta(csrc)
        if stat.S_ISLNK(meta[0]):
            os.symlink(os.readlink(hsrc), hdst)
        elif stat.S_ISREG(meta[0]):
            shutil.copyfile(hsrc, hdst)
            os.chmod(hdst, (meta[0] & 0o777) | 0o600)
        elif stat.S_ISDIR(meta[0]):
            os.mkdir(hdst, 0o755)
            self.note(hdst, meta[0], meta[1])
            for n in sorted(os.listdir(hsrc)):
                self.cp_p(csrc + '/' + n, dest + '/' + n, deref=deref)
            return
        else:
            raise ValueError('cp: unsupported file type ' + src)
        self.note(hdst, meta[0], meta[1])

    def ln_rs(self, target, link):
        t = self.r.canon(target, missing='all')
        d = self.r.canon(link.rsplit('/', 1)[0] or '/', missing='all')
        rel = os.path.relpath(t, d)
        _, hl = self.place(link)
        if os.path.lexists(hl):
            self.log.append('ln: %s exists' % link)
            return
        os.symlink(rel, hl)
        self.note(hl, stat.S_IFLNK | 0o777, 0)

    def ln_s(self, target, link):
        _, hl = self.place(link)
        if os.path.lexists(hl) or self.r.exists(link):
            self.log.append('ln -s: %s exists' % link)
            return
        os.symlink(target, hl)
        self.note(hl, stat.S_IFLNK | 0o777, 0)

    def write(self, target, data, append=False):
        _, hp = self.place(self.dpath(target))
        new = not os.path.lexists(hp)
        with open(hp, 'ab' if append else 'wb') as f:
            f.write(data)
        if new or not append:
            self.note(hp, stat.S_IFREG | 0o644, 0)
        else:
            st = os.lstat(hp)
            old = self.dmeta.get(st.st_ino)
            self.dmeta[st.st_ino] = (old[0] if old else stat.S_IFREG | 0o644, 0, st.st_mtime_ns)

    # -- hook-functions ----------------------------------------------------
    def copy_file(self, kind, src, target=None):
        target = target or src
        if not self.r.isfile(src):
            return 2
        if self.r.isdir(self.dpath(target)):
            target = target + '/' + src.rsplit('/', 1)[-1]
        if USR_MERGED.match(target):
            target = '/usr' + target
        if self.r.exists(self.dpath(target)):
            return 1
        self.mkdir_p(self.dpath(target.rsplit('/', 1)[0]))
        if self.r.islink(src):
            link_target = self.r.readlink_f(src)
            src = link_target
            if USR_MERGED.match(link_target):
                link_target = '/usr' + link_target
            if link_target != target:
                self.ln_rs(self.dpath(link_target), self.dpath(target))
            target = link_target
            if self.r.exists(self.dpath(target)):
                return 0
            self.mkdir_p(self.dpath(target.rsplit('/', 1)[0]))
        self.cp_p(src, self.dpath(target), deref=False)
        return 0

    def ldd(self, src):
        """Library paths `ldd src` prints (sed-filtered as in copy_exec)."""
        c = self.r.canon(src)
        info = elf_read(self.r.host(c))
        if info is None:
            return []
        if info['machine'] != self.a['machine']:
            raise ValueError('ELF architecture mismatch for %s: machine %d does not match %s' %
                             (src, info['machine'], self.arch))
        if not info['dynamic']:
            return []
        if info['interp'] is None and not info['needed']:
            return []  # statically linked / static-pie
        rtld = info['interp'] or self.a['rtld']
        if self.cache is None:
            self.cache = ld_cache(self.r.read('/etc/ld.so.cache'), self.a['cache_flags'])
        loaded = {rtld.rsplit('/', 1)[-1]: rtld, rtld: rtld}
        order = []
        queue = collections.deque([(c, info, [])])
        defaults = ['/lib/' + self.a['triplet'], '/usr/lib/' + self.a['triplet'], '/lib', '/usr/lib']
        while queue:
            path, inf, rpaths = queue.popleft()
            own_rpath = [] if inf['runpath'] else [(x, path) for x in (inf['rpath'] or '').split(':') if x]
            chain = own_rpath + rpaths
            for name in inf['needed']:
                if name in loaded:
                    continue
                found = None
                if '/' in name:
                    found = name if self.r.isfile(name) else None
                else:
                    dirs = [(d, o) for d, o in chain]
                    dirs += [(x, path) for x in (inf['runpath'] or '').split(':') if x]
                    for d, origin in dirs:
                        d = d.replace('$ORIGIN', origin.rsplit('/', 1)[0]).replace('${ORIGIN}', origin.rsplit('/', 1)[0])
                        if self.r.isfile(d + '/' + name):
                            found = d + '/' + name
                            break
                    if found is None and name in self.cache and self.r.isfile(self.cache[name]):
                        found = self.cache[name]
                    if found is None:
                        for d in defaults:
                            if self.r.isfile(d + '/' + name):
                                found = d + '/' + name
                                break
                if found is None:
                    raise ValueError('required ELF library %s not found for %s' % (name, src))
                loaded[name] = found
                order.append(found)
                sub = elf_read(self.r.host(self.r.canon(found)))
                if sub is None or sub['machine'] != self.a['machine']:
                    raise ValueError('bad library ' + found)
                queue.append((found, sub, chain if not inf['runpath'] else own_rpath))
        return order + [rtld]

    def target_elf(self, src, role, data=None):
        """Read native target code; callers keep opaque firmware outside this check."""
        canonical = self.r.canon(src)
        if canonical is None:
            raise ValueError('missing ' + role + ': ' + src)
        info = elf_read(self.r.host(canonical), data=data)
        if info is None:
            raise ValueError(role + ' is not ELF: ' + src)
        if info['machine'] != self.a['machine']:
            raise ValueError('%s ELF architecture mismatch for %s: machine %d does not match %s' %
                             (role, src, info['machine'], self.arch))
        return info

    def copy_exec(self, src, target=None):
        target = target or src
        rc = self.copy_file('binary', src, target)
        if rc:
            return rc - 1
        for x in self.ldd(src):
            nonopt = re.sub(r'/lib/([^/]*/)?(tls|i686|sse2|neon|vfp).*/(lib.*)', r'/lib/\1\3', x)
            nonopt = re.sub(r'-linux-gnu/(tls|i686|sse2|neon|vfp).*/(lib.*)', r'-linux-gnu/\2', nonopt)
            if self.r.exists(nonopt):
                x = nonopt
            if re.search(r'/libpthread\.so\.', x):
                for lib in self.r.glob(x.rsplit('/', 1)[0], 'libgcc_s.so.[1-9]'):
                    rc = self.copy_exec(lib)
                    if rc:
                        return rc
            rc = self.copy_file('binary', x)
            if rc and rc != 1:
                return rc - 1
        return 0

    def kmod_tool(self, name, args):
        if name != 'modprobe':
            raise ValueError('unsupported declared kmod operation ' + name)
        return subprocess.run([self.modprobe] + args, env=ENV, capture_output=True)

    def modprobe_config(self):
        return [a for d in ('/etc/modprobe.d', '/run/modprobe.d', '/usr/local/lib/modprobe.d', '/lib/modprobe.d')
                for a in (['-C', self.r.host(self.r.canon(d))] if self.r.isdir(d) else [])]

    def manual_add_modules(self, modules):
        if not modules:
            return
        common = ['-d', self.r.h, '--all', '--set-version=' + self.version] + self.modprobe_config()
        out = b''
        for extra in (['--ignore-install'], []):
            out += self.kmod_tool('modprobe', common + extra + ['--quiet', '--show-depends'] + modules).stdout
        for line in out.decode().splitlines():
            f = line.split()
            if len(f) < 2 or f[0] != 'insmod':
                continue
            kmod = f[1]
            if not kmod.startswith(self.r.h + '/'):
                raise ValueError('modprobe outside root: ' + line)
            kmod = kmod[len(self.r.h):]
            if self.copy_file('module', kmod):
                continue
            module_path = self.r.host(self.r.canon(kmod))
            info = self.target_elf(kmod, 'kernel module', data=self.module_bytes(module_path))
            modinfo = elf_section(info, '.modinfo') or b''
            for item in modinfo.split(b'\0'):
                if item.startswith(b'firmware='):
                    for fw in item[9:].decode().split():
                        if not self.add_firmware(fw):
                            self.log.append('missing firmware %s for %s' % (fw, kmod))

    def module_bytes(self, path):
        """Read module ELF data for modinfo without executing target programs."""
        if str(path).endswith('.xz'):
            with lzma.open(path, 'rb') as stream:
                return stream.read()
        if str(path).endswith('.zst'):
            if not self.zstd:
                raise ValueError('zstd module requires a declared zstd executable')
            return self.run([self.zstd, '-q', '-d', '-c', str(path)]).stdout
        if str(path).endswith('.gz'):
            with gzip.open(path, 'rb') as stream:
                return stream.read()
        with open(path, 'rb') as stream:
            return stream.read()

    def decompress_modules(self, directory):
        """mkinitramfs 0.142 decompresses .xz/.zst modules before depmod."""
        for parent, _, names in os.walk(directory):
            for name in sorted(names):
                if not name.endswith(('.ko.xz', '.ko.zst')):
                    continue
                source = os.path.join(parent, name)
                output = source.rsplit('.', 1)[0]
                if os.path.lexists(output):
                    raise ValueError('decompressed module path already exists: ' + output)
                st = os.stat(source)
                meta = self.dmeta.get(st.st_ino)
                with open(output, 'wb') as stream:
                    stream.write(self.module_bytes(source))
                os.chmod(output, stat.S_IMODE(st.st_mode))
                os.utime(output, (st.st_atime, st.st_mtime))
                self.note(output, meta[0] if meta else stat.S_IFREG | stat.S_IMODE(st.st_mode),
                          meta[1] if meta else 0)
                os.unlink(source)

    def add_firmware(self, fw):
        v = self.version
        locs = ['/lib/firmware/updates/%s/%s' % (v, fw), '/lib/firmware/updates/' + fw,
                '/lib/firmware/%s/%s' % (v, fw), '/lib/firmware/' + fw]
        if any(self.r.exists(self.dpath(l)) for l in locs):
            return True
        for l in locs:
            if self.r.exists(l):
                self.copy_file('firmware', l)
                return True
        return False

    def force_load(self, module, args):
        self.manual_add_modules([module])
        self.write('/conf/modules', ('%s %s\n' % (module, args)).encode(), append=True)

    def add_modules_from_file(self, path):
        for line in self.r.read(path).decode().split('\n'):
            if not line or line[0] == '#':
                continue
            f = line.strip().split(None, 1)
            if not f:
                continue
            self.force_load(f[0], f[1].strip() if len(f) > 1 else '')

    def copy_modules_dir(self, d, excludes=()):
        base = self.modulesdir + '/' + d
        if not self.r.isdir(base):
            return
        pats = [e + '*' if e.endswith('.ko') else e for e in excludes]
        mods = []

        def walk(cpath):
            hp = self.r.host(self.r.canon(cpath))
            for n in sorted(os.listdir(hp)):
                if any(fnmatch.fnmatchcase(n, p) for p in pats):
                    continue
                st = os.lstat(hp + '/' + n)
                if fnmatch.fnmatchcase(n, '*.ko*'):
                    mods.append(n.split('.', 1)[0])
                if stat.S_ISDIR(st.st_mode):
                    walk(cpath + '/' + n)
        walk(base)
        self.manual_add_modules(mods)

    def auto_add_modules(self):
        cmd = self.copy_modules_dir
        mods = 'btrfs ext2 ext3 ext4 f2fs isofs jfs reiserfs udf xfs nfs nfsv2 nfsv3 nfsv4 af_packet atkbd i8042 psmouse virtio_pci virtio_mmio'.split()
        cmd('kernel/drivers/usb/host', ['hwa-hc.ko', 'sl811_cs.ko', 'sl811-hcd.ko', 'u132-hcd.ko', 'whci-hcd.ko'])
        for d in ('c67x00', 'chipidea', 'dwc2', 'dwc3', 'isp1760', 'musb', 'renesas_usbhs', 'typec/tcpm'):
            cmd('kernel/drivers/usb/' + d)
        mods += ['extcon-usb-gpio', 'extcon-usbc-cros-ec', 'axp20x_usb_power']
        cmd('kernel/drivers/input/keyboard')
        cmd('kernel/drivers/hid', ['hid-*ff.ko', 'hid-a4tech.ko', 'hid-cypress.ko', 'hid-dr.ko', 'hid-elecom.ko', 'hid-gyration.ko',
                                   'hid-icade.ko', 'hid-kensington.ko', 'hid-kye.ko', 'hid-lcpower.ko', 'hid-magicmouse.ko',
                                   'hid-multitouch.ko', 'hid-ntrig.ko', 'hid-petalynx.ko', 'hid-picolcd.ko', 'hid-pl.ko',
                                   'hid-ps3remote.ko', 'hid-quanta.ko', 'hid-roccat-ko*.ko', 'hid-roccat-pyra.ko',
                                   'hid-saitek.ko', 'hid-sensor-hub.ko', 'hid-sony.ko', 'hid-speedlink.ko', 'hid-tivo.ko',
                                   'hid-twinhan.ko', 'hid-uclogic.ko', 'hid-wacom.ko', 'hid-waltop.ko', 'hid-wiimote.ko',
                                   'hid-zydacron.ko'])
        mods += ['cros_ec_spi']
        for d in ('bus', 'clk', 'gpio', 'i2c/busses', 'i2c/muxes', 'mfd', 'pci/controller', 'phy', 'pinctrl',
                  'regulator', 'reset', 'spi', 'usb/phy', 'rtc'):
            cmd('kernel/drivers/' + d)
        # net
        cmd('kernel/drivers/net', ['appletalk', 'arcnet', 'bonding', 'can', 'cdc-phonet.ko', 'cdc_mbim.ko', 'cdc_subset.ko',
                                   'cx82310_eth.ko', 'dummy.ko', 'gl620a.ko', 'hamradio', 'hippi', 'hso.ko',
                                   'huawei_cdc_ncm.ko', 'ifb.ko', 'ipheth.ko', 'irda', 'kalmia.ko', 'lg-vl600.ko',
                                   'macvlan.ko', 'macvtap.ko', 'net1080.ko', 'pcmcia', 'plusb.ko', 'qmi_wwan.ko',
                                   'sb1000.ko', 'sierra_net.ko', 'team', 'tokenring', 'tun.ko', 'veth.ko', 'wan', 'wimax',
                                   'wireless', 'xen-netback.ko', 'zaurus.ko'])
        mods += ['nvmem-imx-ocotp']
        cmd('kernel/drivers/ide')
        cmd('kernel/drivers/scsi')
        cmd('kernel/drivers/ufs')
        mods += 'mptfc mptsas mptscsih mptspi zfcp'.split()
        cmd('kernel/drivers/block')
        cmd('kernel/drivers/nvme')
        mods += ['vmd']
        cmd('kernel/drivers/ata')
        mods += 'dasd_diag_mod dasd_eckd_mod dasd_fba_mod'.split()
        mods += 'firewire-ohci firewire-sbp2'.split()
        cmd('kernel/drivers/mmc')
        cmd('kernel/drivers/usb/storage')
        mods += 'rockchipdrm pwm-cros-ec pwm_bl pwm-rockchip panel-simple analogix-anx6345 pwm-sun4i sun4i-drm sun8i-mixer panel-edp pwm_imx27 nwl-dsi ti-sn65dsi86 imx-dcss mux-mmio mxsfb imx8mq-interconnect'.split()
        self.manual_add_modules(mods)

    def hidden_dep_add_modules(self):
        mdir = self.dpath(self.modulesdir)
        names = [l for l in self.r.read(mdir + '/modules.builtin').decode().split('\n') if l]
        kdir = mdir + '/kernel'
        if self.r.isdir(kdir):
            for dp, dns, fns in os.walk(self.r.host(self.r.canon(kdir))):
                dns.sort()
                names += [n for n in sorted(fns + dns) if fnmatch.fnmatchcase(n, '*.ko*')]
        table = {'libcrc32c': ['crc32c'], 'ubifs': ['deflate', 'zlib', 'lzo'], 'btrfs': ['crc32c'], 'f2fs': ['crc32'],
                 'mlx4_core': ['mlx4_ib'], 'mlx5_core': ['mlx5_ib'], 'i8042': ['psmouse'], 'nvme': ['vmd'],
                 'spi-rockchip': ['pl330'], 'dw_mmc-rockchip': ['rockchip-io-domain', 'io-domain']}
        mods = []
        for n in names:
            mods += table.get(n.rsplit('/', 1)[-1].split('.', 1)[0], [])
        self.manual_add_modules(mods)

    def add_builtin_firmware(self):
        p = self.modulesdir + '/modules.builtin.modinfo'
        if not self.r.exists(p):
            raise ValueError('missing ' + p)
        for item in self.r.read(p).split(b'\0'):
            m = re.match(rb'^([^=]*)\.firmware=(.*)$', item)
            if m:
                fw = m.group(2).decode().strip().split(None, 1)
                if fw and not self.add_firmware(fw[0]):
                    self.log.append('missing builtin firmware ' + fw[0])

    # -- hooks ---------------------------------------------------------------
    def hook_noop(self):
        pass

    def hook_fsck(self):
        # The fsck hooks only act for / and /usr entries with a probeable
        # type; a chroot build has neither (Make baseline: no fsck/logsave).
        fstab = '/etc/fstab'
        if self.r.isfile(fstab):
            for line in self.r.read(fstab).decode().splitlines():
                f = line.split()
                if f and not f[0].startswith('#') and len(f) > 2 and f[1] in ('/', '/usr') and f[2] not in ('auto', 'none'):
                    raise NotImplementedError('fsck hook would install fsck.' + f[2])

    def hook_kdump_tools(self):
        if self.confdir != '/var/lib/kdump/initramfs-tools':
            return
        if not self.r.isfile('/etc/kdump/sysctl.conf'):
            self.log.append('kdump-tools hook: no /etc/kdump/sysctl.conf')
        self.copy_file('config', '/etc/kdump/sysctl.conf')

    def hook_keymap(self):
        if self.conf.get('KEYMAP', 'n') not in ('n', 'N', 'no', 'NO', 'No'):
            raise NotImplementedError('KEYMAP=y')

    def hook_resume(self):
        if self.conf.get('RESUME'):
            raise NotImplementedError('RESUME is set')

    def hook_thermal(self):
        if self.arch == 'amd64':
            self.manual_add_modules(['fan'])
            self.manual_add_modules(['thermal'])

    def hook_kmod(self):
        self.copy_exec('/usr/bin/kmod')
        for p in ('/usr/sbin/modprobe', '/usr/sbin/rmmod'):
            self.cp_p(p, self.dpath('/usr/sbin/'), deref=False)
        self.mkdir_p(self.dpath('/usr/lib/modprobe.d/'))
        for p in self.r.glob('/usr/lib/modprobe.d', '*'):
            self.cp_p(p, self.dpath('/usr/lib/modprobe.d/'), deref=False)
        config = self.r.read('/boot/config-' + self.version).decode()
        if not re.search(r'^CONFIG_MODULE_DECOMPRESS=y$', config, re.M):
            directory = self.r.host(self.r.canon(self.dpath(self.modulesdir)))
            for extension, library in (('.xz', 'liblzma.so.5'), ('.zst', 'libzstd.so.1')):
                if any(name.endswith(extension) for _, _, names in os.walk(directory) for name in names):
                    path = '/usr/lib/' + self.a['triplet'] + '/' + library
                    if not self.r.isfile(path):
                        raise ValueError('kmod decompression library missing: ' + path)
                    self.copy_exec(path)


    def hook_udev(self):
        self.mkdir_p(self.dpath('/usr/lib/systemd'))
        self.copy_exec('/usr/lib/systemd/systemd-udevd', '/usr/lib/systemd')
        self.copy_exec('/usr/bin/udevadm', '/usr/bin')
        # systemd >=256 loads libkmod through dlopen, not DT_NEEDED.
        for directory in self.r.glob('/usr/lib', '*'):
            if self.r.isdir(directory):
                for library in self.r.glob(directory, 'libkmod.so.*'):
                    if self.r.isfile(library):
                        self.copy_exec(library)
        if self.r.exists('/etc/udev/udev.conf'):
            self.mkdir_p(self.dpath('/etc/udev'))
            self.cp_p('/etc/udev/udev.conf', self.dpath('/etc/udev/'))
        if self.r.exists('/usr/lib/udev/hwdb.bin'):
            self.mkdir_p(self.dpath('/usr/lib/udev'))
            self.cp_p('/usr/lib/udev/hwdb.bin', self.dpath('/usr/lib/udev/hwdb.bin'))
        network = self.dpath('/usr/lib/systemd/network/')
        self.mkdir_p(network)
        for top in ('/usr/lib/systemd/network', '/etc/systemd/network'):
            if top.startswith('/etc') and not self.r.isdir(top):
                continue
            for path in self.find_L(top):
                if fnmatch.fnmatchcase(path.rsplit('/', 1)[-1], '*.link*'):
                    self.copy_network_entry(path, network)
        rules_directory = self.dpath('/usr/lib/udev/rules.d/')
        self.mkdir_p(rules_directory)
        for rules in ('50-firmware.rules', '50-udev-default.rules', '60-block.rules', '60-persistent-storage.rules',
                      '61-persistent-storage-android.rules', '71-seat.rules', '73-special-net-names.rules',
                      '75-net-description.rules', '80-net-setup-link.rules', '80-drivers.rules'):
            for directory in ('/etc/udev/rules.d/', '/usr/lib/udev/rules.d/'):
                if self.r.exists(directory + rules):
                    self.cp_p(directory + rules, rules_directory)
                    break
        if self.r.exists('/usr/sbin/chzdev'):
            raise NotImplementedError('chzdev-owned udev rules require a reviewed target-specific handler')
        for path in self.r.glob('/etc/udev/rules.d', '*.rules'):
            if self.r.exists(path) and not self.r.exists('/usr/lib/' + path[len('/etc/'):]):
                self.cp_p(path, rules_directory)
        for program in ('ata_id', 'scsi_id'):
            self.copy_exec('/usr/lib/udev/' + program, '/usr/lib/udev')
        self.copy_exec('/sbin/blkid', '/sbin')

    def copy_network_entry(self, source, directory):
        """cp -r -p into an existing .link.d tree merges package and local files."""
        if self.r.isdir(source):
            destination = directory.rstrip('/') + '/' + source.rsplit('/', 1)[-1]
            self.mkdir_p(destination)
            canonical = self.r.canon(source)
            for name in sorted(os.listdir(self.r.host(canonical))):
                self.copy_network_entry(source.rstrip('/') + '/' + name, destination)
            mode, mtime, _, _ = self.r.rmeta(canonical)
            self.note(self.r.host(self.r.canon(destination)), mode, mtime)
        else:
            self.cp_p(source, directory)

    def find_L(self, top):
        out = []
        hp = self.r.host(self.r.canon(top))
        for n in sorted(os.listdir(hp)):
            p = top + '/' + n
            out.append(p)
            if self.r.isdir(p):
                out += self.find_L(p)
        return out

    def hook_zz_busybox(self):
        if self.conf.get('BUSYBOX') == 'n':
            return
        bb = '/usr/bin/busybox'
        sh = self.dpath('/bin/sh')
        if self.r.isfile(sh) and self.r.read(sh) == self.r.read(bb):
            os.unlink(self.r.host(self.r.canon(sh)))
        if self.r.lexists(self.dpath('/bin/busybox')):
            os.unlink(self.place(self.dpath('/bin/busybox'))[1])
        self.copy_exec(bb, '/bin/busybox')
        status = self.r.read('/var/lib/dpkg/status').decode()
        versions = []
        for paragraph in status.split('\n\n'):
            if re.search(r'^Package: busybox$', paragraph, re.M):
                versions += re.findall(r'^Version: (.+)$', paragraph, re.M)
        # Debian binNMU suffixes change the binary revision, not its source/config.
        source_versions = [re.sub(r'\+b[0-9]+$', '', version) for version in versions]
        if source_versions != [self.busybox_manifest['version']]:
            raise ValueError('BusyBox deb/source version mismatch: %r (expected %s)' %
                             (versions, self.busybox_manifest['version']))
        listing = self.busybox_manifest['applets']
        self.busybox_applets = listing
        src = self.r.host(self.r.canon(self.dpath('/bin/busybox')))
        for alias in listing:
            alias = alias.lstrip('/') if alias.startswith('/') else alias
            if alias.startswith('usr/'):
                alias = alias[4:]
            elif '/' not in alias:
                alias = 'bin/' + alias
            if not self.r.exists(self.dpath(alias)):
                os.link(src, self.place(self.dpath(alias))[1])

    def hook_dmsetup(self):
        self.mkdir_p(self.dpath('/usr/lib/udev/rules.d/'))
        for rules in ('55-dm.rules', '60-persistent-storage-dm.rules', '95-dm-notify.rules'):
            for d in ('/etc/udev/rules.d/', '/usr/lib/udev/rules.d/'):
                if self.r.exists(d + rules):
                    self.cp_p(d + rules, self.dpath('/usr/lib/udev/rules.d/'))
                    break
            else:
                raise RuntimeError('dmsetup hook: missing ' + rules)
        self.copy_exec('/usr/sbin/dmsetup')
        self.manual_add_modules(['dm_mod'])

    def hook_klibc_utils(self):
        for src in self.r.glob('/usr/lib/klibc/bin', '*'):
            command = src.rsplit('/', 1)[-1]
            if command in ('gzip', 'kinit', 'zcat'):
                continue
            if not self.r.exists(self.dpath('/sbin/' + command)):
                self.target_elf(src, 'klibc program')
                self.cp_p(src, self.dpath('/bin'), no_clobber=True)
        libs = self.r.glob('/usr/lib', 'klibc-*.so')
        if not libs:
            raise RuntimeError('klibc-utils hook: no /usr/lib/klibc-*.so')
        for src in libs:
            self.target_elf(src, 'klibc library')
            self.cp_p(src, self.dpath('/lib'))

    def hook_mke2fs(self):
        self.copy_exec('/usr/sbin/mke2fs', '/usr/local/sbin/')
        for p in ('/sbin/sfdisk', '/sbin/fdisk', '/sbin/resize2fs', '/sbin/tune2fs', '/sbin/findfs'):
            self.copy_exec(p)
        for kind in ('mkfs', 'fsck'):
            for t in ('ext4', 'ext3'):
                prog = '/sbin/%s.%s' % (kind, t)
                if self.r.islink(prog):
                    link = self.r.readlink_f(prog)
                    self.copy_exec(link)
                    self.ln_s('/usr/local/sbin/' + link.rsplit('/', 1)[-1] if kind == 'mkfs' else link, self.dpath(prog))
                elif self.r.is_exec(prog):
                    self.copy_exec(prog)
                else:
                    self.log.append('mke2fs hook: %s missing' % prog)

    def hook_union_fsck(self):
        # Sources the fsck hook (no-op, see hook_fsck) and probes /host in
        # /etc/mtab (== the build host's /proc/mounts); a /host mount never
        # exists while building, so no fsck program is added.
        self.hook_fsck()

    def hook_setfacl(self):
        self.copy_exec('/usr/bin/setfacl', '/sbin/setfacl')

    def hook_pzstd(self):
        self.copy_exec('/usr/bin/pzstd', '/usr/bin')

    def hook_file(self):
        self.copy_exec('/usr/bin/file', '/usr/bin')
        self.copy_exec('/usr/lib/file/magic.mgc', '/etc')

    def hook_uboot_utils(self):
        self.copy_exec('/usr/sbin/ubiattach', '/sbin/ubiattach')
        self.copy_exec('/usr/sbin/mtdinfo', '/sbin/mtdinfo')

    def hook_prereqs(self, name):
        if name == 'dmsetup':
            return ['udev']
        if name == 'klibc-utils':
            return [] if self.conf.get('BUSYBOX') == 'n' or not self.busyboxdir else ['zz-busybox']
        return []

    def run_hooks(self, initdir):
        if not self.r.isdir(initdir):
            return
        initlist = []
        for p in self.r.glob(initdir, '*'):
            name = p.rsplit('/', 1)[-1]
            if not SCRIPT_NAME.match(name) or self.r.isdir(p) or not self.r.is_exec(p):
                continue
            digest = sha256(self.r.host(self.r.canon(p)))
            if HOOK_PINS.get(name) != digest:
                raise ValueError('unpinned initramfs hook %s (sha256 %s): review and extend mkinitramfs.py' % (p, digest))
            initlist.append(name)
        pairs = []
        for n in initlist:
            pairs.append((n, n))
            pairs += [(pr, n) for pr in self.hook_prereqs(n)]
        for n in gnu_tsort(pairs):
            if n not in initlist:
                continue
            self.hooks_run.append(initdir + '/' + n)
            getattr(self, 'hook_' + n.replace('-', '_'))()

    # -- scripts ORDER ---------------------------------------------------------
    def script_source(self, scriptdir, name):
        c = self.confdir + scriptdir + '/' + name
        return c if self.r.isfile(c) else '/usr/share/initramfs-tools' + scriptdir + '/' + name

    def cache_run_scripts(self, scriptdir):
        initdir = self.D + scriptdir
        self.write(scriptdir + '/ORDER', b'')
        initlist = []
        for p in self.r.glob(initdir, '*'):
            name = p.rsplit('/', 1)[-1]
            if not SCRIPT_NAME.match(name) or self.r.isdir(p):
                continue
            if not self.r.is_exec(self.script_source(scriptdir, name)):
                continue
            initlist.append(name)
        pairs = []
        for n in initlist:
            pairs.append((n, n))
            src = self.script_source(scriptdir, n)
            pairs += [(pr, n) for pr in script_prereqs(self.r.read(src).decode(errors='replace'), src).split()]
        lines = []
        for n in gnu_tsort(pairs):
            if self.r.isfile(initdir + '/' + n):
                lines.append('%s/%s "$@"\n[ -e /conf/param.conf ] && . /conf/param.conf\n' % (scriptdir, n))
        self.write(scriptdir + '/ORDER', ''.join(lines).encode())
        self.orders[scriptdir] = ''.join(lines)

    # -- main ------------------------------------------------------------------
    def build(self, confdir):
        r = self.r
        self.confdir = confdir
        for rel, want in PINS.items():
            got = sha256(r.host(r.canon('/' + rel)))
            if got != want:
                raise ValueError('unpinned %s (sha256 %s)' % (rel, got))
        conf = shell_conf(r.read(confdir + '/initramfs.conf').decode(), confdir + '/initramfs.conf')
        extra = []
        candidates = [p for p in r.glob('/usr/share/initramfs-tools/conf.d', '*')
                      if not r.exists(confdir + '/conf.d/' + p.rsplit('/', 1)[-1])]
        candidates += r.glob(confdir + '/conf.d', '*')
        for p in candidates:
            n = p.rsplit('/', 1)[-1]
            if r.exists(p) and CONF_NAME.match(n) and not re.search(r'\.dpkg-.*$', n):
                if r.isdir(p):
                    continue
                extra.append(p)
                conf.update(shell_conf(r.read(p).decode(), p))
        self.busyboxdir = '/bin'  # conf-hooks.d/busybox (pinned)
        self.conf = conf
        unsupported = {k: conf.get(k) for k in ('UMASK', 'ROOT', 'RESUME', 'DEVICE', 'NFSROOT')
                       if conf.get(k) not in (None, '', 'auto')}
        self.compression_level = compression_level({'compression': self.spec_compression, 'compression_level': self.spec_compression_level}, conf)
        conf['MODULES'] = self.spec_modules
        conf['COMPRESS'] = self.spec_compression
        if unsupported or conf.get('MODULES') not in ('most', 'list') or conf.get('COMPRESS') not in ('zstd', 'gzip', 'xz'):
            raise NotImplementedError('initramfs.conf settings not replayed: %r %r' % (unsupported, conf))
        v = self.version
        if not re.search(r'^CONFIG_RD_' + {'zstd': 'ZSTD', 'gzip': 'GZIP', 'xz': 'XZ'}[conf['COMPRESS']] + '=y$', r.read('/boot/config-' + v).decode(), re.M):
            raise ValueError('kernel lacks configured initrd decompressor')
        self.modulesdir = '/lib/modules/' + v
        if not r.exists(self.modulesdir + '/modules.dep'):
            raise ValueError('missing modules.dep')
        hd = r.host(r.canon('/var/tmp', missing='all'))
        os.makedirs(hd, exist_ok=True)
        os.mkdir(r.host(r.canon('/var/tmp')) + '/mkinitramfs_bazel', 0o755)
        self.note(r.host(r.canon(self.D)), stat.S_IFDIR | 0o755, 0)
        for d in ['/bin'] + r.glob('/', 'lib*') + ['/sbin']:
            self.mkdir_p(self.dpath('/usr' + d))
            self.ln_s('usr' + d, self.dpath(d))
        for d in ('conf/conf.d', 'etc', 'run', 'scripts', self.modulesdir):
            self.mkdir_p(self.dpath(d))
        for x in ('modules.builtin', 'modules.builtin.bin', 'modules.builtin.modinfo', 'modules.order'):
            if r.isfile(self.modulesdir + '/' + x):
                self.cp_p(self.modulesdir + '/' + x, self.dpath(self.modulesdir + '/' + x))
        for x in [confdir + '/modules'] + r.glob('/usr/share/initramfs-tools/modules.d', '*'):
            if r.isfile(x):
                self.add_modules_from_file(x)
        if self.spec_modules == "most":
            self.auto_add_modules()
        self.hidden_dep_add_modules()
        self.add_builtin_firmware()
        self.cp_p('/usr/share/initramfs-tools/init', self.dpath('/init'))
        for top, maxdepth in (('/usr/share/initramfs-tools/scripts', None), (confdir + '/scripts', 2)):
            for b in self.find_scripts(top, maxdepth):
                d = b.rsplit('/', 1)[0] if '/' in b else ''
                ddir = self.dpath('/scripts/' + d) if d else self.dpath('/scripts')
                if not r.isdir(ddir):
                    self.mkdir_p(ddir)
                self.cp_p(top + '/' + b, ddir + '/')
        self.write('/conf/arch.conf', ('DPKG_ARCH=%s\n' % self.arch).encode())
        self.cp_p(confdir + '/initramfs.conf', self.dpath('/conf'))
        self.write('/conf/initramfs.conf', r.read(confdir + '/initramfs.conf') + ('\nMODULES=%s\nCOMPRESS=%s\nCOMPRESSLEVEL=%d\n' % (self.spec_modules, self.spec_compression, self.compression_level)).encode())
        for p in extra:
            self.copy_file('config', p, '/conf/conf.d')
            # These settings are explicitly owned by configurable rule attrs.
            selected = self.dpath('/conf/conf.d/' + p.rsplit('/', 1)[-1])
            text = r.read(selected).decode()
            self.write('/conf/conf.d/' + p.rsplit('/', 1)[-1], re.sub(r'(?m)^(?:export\s+)?(?:MODULES|COMPRESS|COMPRESSLEVEL)=.*$', '', text).encode())
        self.write('/etc/fstab', b'')
        self.ln_s('/proc/mounts', self.dpath('/etc/mtab'))
        self.copy_exec('/sbin/modprobe', '/sbin')
        self.copy_exec('/sbin/rmmod', '/sbin')
        self.mkdir_p(self.dpath('/etc/modprobe.d'))
        self.mkdir_p(self.dpath('/lib/modprobe.d'))
        for p in r.glob('/etc/modprobe.d', '*.conf') + r.glob('/lib/modprobe.d', '*.conf'):
            if r.exists(p) or r.islink(p):
                self.copy_file('config', p)
        self.run_hooks('/usr/share/initramfs-tools/hooks')
        self.run_hooks(confdir + '/hooks')
        sdir = r.host(r.canon(self.dpath('/scripts')))
        for dp, dns, _ in os.walk(sdir):
            for n in sorted(dns):
                self.cache_run_scripts('/scripts/' + os.path.relpath(os.path.join(dp, n), sdir))
        mdir = r.host(r.canon(self.dpath(self.modulesdir)))
        self.decompress_modules(mdir)
        # Use the declared native depmod. Host depmod.d must never affect
        # indexes for the target module tree.
        dconf = [d for d in ('/etc/depmod.d', '/lib/depmod.d', '/usr/lib/depmod.d', '/usr/local/lib/depmod.d', '/run/depmod.d')
                 if r.isdir(d) and r.glob(d, '*.conf')]
        if dconf:
            raise NotImplementedError('image depmod.d configuration: %r' % dconf)
        empty = os.path.join(self.work, 'empty-depmod.d')
        os.makedirs(empty, exist_ok=True)
        self.run([self.depmod, '-a', '-b', r.host(r.canon(self.D)), '-C', empty, v])
        for p in r.glob('/etc', 'ld.so.conf*'):
            self.cp_p(p, self.dpath('/etc/'), deref=False)
        self.write('/etc/ld.so.cache', generate_cache(r.host(r.canon(self.D)), self.arch))
        lc = self.dpath('/var/cache/ldconfig')
        if r.isdir(lc):
            hp = r.host(r.canon(lc))
            if os.path.lexists(hp + '/aux-cache'):
                os.unlink(hp + '/aux-cache')
            if not os.listdir(hp):
                os.rmdir(hp)
        if r.exists(confdir + '/DSDT.aml'):
            self.copy_file('DSDT', confdir + '/DSDT.aml')

    def find_scripts(self, top, maxdepth):
        c = self.r.canon(top)
        if c is None:
            return []
        base = self.r.host(c)
        out = []

        def walk(rel, depth):
            hp = base + ('/' + rel if rel else '')
            for n in sorted(os.listdir(hp)):
                if maxdepth is not None and len(n) >= 2 and n.startswith('.'):
                    continue
                p = (rel + '/' + n) if rel else n
                st = os.lstat(hp + '/' + n)
                if stat.S_ISREG(st.st_mode) and SCRIPT_NAME.match(n):
                    out.append(p)
                if stat.S_ISDIR(st.st_mode) and (maxdepth is None or depth + 1 < maxdepth):
                    walk(p, depth + 1)
        walk('', 0)
        return out

    # -- shared declarative newc assembler ------------------------------------
    def cpio(self, out):
        base = self.r.host(self.r.canon(self.D))

        def metadata(hp, st):
            m = self.dmeta.get(st.st_ino)
            if m is not None and stat.S_IFMT(m[0]) == stat.S_IFMT(st.st_mode) and \
                    (stat.S_ISDIR(st.st_mode) or m[2] == st.st_mtime_ns):
                return m[0], m[1]
            mode = st.st_mode & ~0o7000
            self.log.append('generated: %s %o' % (os.path.relpath(hp, base), mode))
            return mode, 0

        return write_newc(base, out, metadata)


def compression_level(spec, conf=None):
    level = spec.get('compression_level', -1)
    if level == -1:
        configured = (conf or {}).get('COMPRESSLEVEL', '')
        # initramfs-tools 0.142 uses zstd -9 and gzip/xz's default level 6.
        level = int(configured) if configured else {'zstd': 9, 'gzip': 6, 'xz': 6}[spec.get('compression', 'zstd')]
    return level


def compress_initrd(source_path, output, spec):
    compression = spec.get('compression', 'zstd')
    level = compression_level(spec)
    with open(source_path, 'rb') as source, open(output, 'wb') as destination:
        if compression == 'gzip':
            if not spec.get('pigz'):
                raise ValueError('gzip compression requires a declared pigz executable')
            subprocess.run([spec['pigz'], '-n', '-' + str(level), '-c'], stdin=source, stdout=destination,
                           check=True, env=ENV)
        elif compression == 'xz':
            with lzma.LZMAFile(destination, mode='wb', preset=level, check=lzma.CHECK_CRC32) as compressed:
                shutil.copyfileobj(source, compressed, 1024 * 1024)
        elif compression == 'zstd':
            if not spec.get('zstd'):
                raise ValueError('zstd compression requires a declared zstd executable')
            result = subprocess.run([spec['zstd'], '-q', '-' + str(level), '-c'], stdin=source, stdout=destination,
                                    stderr=subprocess.PIPE, env=ENV)
            if result.returncode:
                raise RuntimeError('zstd failed: ' + result.stderr.decode())
        else:
            raise ValueError('unsupported initramfs compression: ' + compression)


def read_kernel_metadata(path):
    metadata = read_boot_metadata(path)
    if metadata.get('architecture') not in ARCH:
        raise ValueError('unsupported kernel image metadata architecture')
    return metadata


def prepare_module_indexes(root, mk, work, release, module_path, module_indexes):
    # Debian image.postinst runs depmod before linux-run-hooks invokes
    # mkinitramfs. Reproduce that installation effect in this same stage.
    dconf = [directory for directory in ('/etc/depmod.d', '/lib/depmod.d', '/usr/lib/depmod.d', '/usr/local/lib/depmod.d', '/run/depmod.d')
             if root.isdir(directory) and root.glob(directory, '*.conf')]
    if dconf:
        raise NotImplementedError('image depmod.d configuration: %r' % dconf)
    empty = os.path.join(work, 'empty-host-depmod.d')
    os.mkdir(empty)
    mk.run([mk.depmod, '-b', root.h, '-C', empty, release])
    if module_indexes is not None:
        with tarfile.open(module_indexes, 'w', format=tarfile.PAX_FORMAT) as archive:
            for source in root.glob(module_path, 'modules.*'):
                if root.isfile(source):
                    content = root.read(source)
                    member = tarfile.TarInfo(source.lstrip('/'))
                    member.mode, member.size = 0o644, len(content)
                    archive.addfile(member, io.BytesIO(content))


def build_initrd(spec, output, report, module_indexes=None, kernel_links=None):
    metadata = read_kernel_metadata(spec['kernel_metadata'])
    spec = dict(spec, arch=metadata['architecture'], kernel_release=metadata['kernel_release'])
    if spec.get('compression', 'zstd') not in ('zstd', 'gzip', 'xz'):
        raise ValueError('unsupported initramfs compression')
    os.umask(0o022)
    with tempfile.TemporaryDirectory(prefix='mkinitramfs-', dir='.') as tmp:
        work = os.path.abspath(tmp)
        root = Rootfs(os.path.join(work, 'root'))
        os.mkdir(root.h)
        for layer in spec['layers']:
            root.apply_layer(layer, skip_prefixes=('var/lib/docker', 'dev'))
        release = metadata['kernel_release']
        for entry in metadata['boot'].values():
            source = root.canon('/' + entry['path'])
            if source is None or sha256(root.host(source)) != entry['sha256']:
                raise ValueError('rootfs boot file differs from kernel producer metadata: ' + entry['path'])
        module_directory = metadata.get('module_directory')
        if not isinstance(module_directory, str):
            raise ValueError('invalid producer module directory')
        module_path = '/' + module_directory
        standard_path = '/lib/modules/' + release
        if not root.isdir(module_path) or os.path.basename(module_path) != release or \
                not root.isdir(standard_path) or root.canon(module_path) != root.canon(standard_path):
            raise ValueError('rootfs lacks matching producer module directory')
        confdir = spec.get('config_dir', '/etc/initramfs-tools')
        if not confdir.startswith('/') or '..' in confdir.split('/'):
            raise ValueError('config_dir must be an absolute rootfs path')
        for extra in spec.get('extra_config', []):
            name = os.path.basename(extra)
            if not CONF_NAME.fullmatch(name):
                raise ValueError('invalid initramfs configuration fragment name: ' + name)
            directory = root.host(root.canon(confdir + '/conf.d', missing='all'))
            os.makedirs(directory, exist_ok=True)
            destination = os.path.join(directory, name)
            relative = os.path.relpath(destination, root.h)
            # Replace the final entry itself instead of following a rootfs symlink.
            if os.path.lexists(destination):
                root._remove(destination)
            root._drop_meta(relative)
            with open(extra, 'rb') as source, open(destination, 'xb') as target:
                shutil.copyfileobj(source, target, 1 << 20)
                os.fchmod(target.fileno(), 0o644)
            root.meta[relative] = (stat.S_IFREG | 0o644, 0, 0, 0)
        mk = Mkinitramfs(root, spec, work)
        prepare_module_indexes(root, mk, work, release, module_path, module_indexes)
        mk.build(confdir)
        if kernel_links is not None:
            # linux-base 4.12.1 linux-update-symlinks defaults for this fresh
            # one-kernel VS installation (image_dest=/, do_symlinks=yes).
            with tarfile.open(kernel_links, 'w', format=tarfile.PAX_FORMAT) as archive:
                for name, target in [('vmlinuz', metadata['boot']['vmlinuz']['path']),
                                     ('initrd.img', 'boot/initrd.img-' + release)]:
                    member = tarfile.TarInfo(name)
                    member.type, member.mode, member.linkname = tarfile.SYMTYPE, 0o777, target
                    archive.addfile(member)
        spec['compression_level'] = mk.compression_level
        cpio_path = os.path.join(work, 'initrd.cpio')
        listing = mk.cpio(cpio_path)
        compress_initrd(cpio_path, output, spec)
        seen_inodes = set()
        allocated = 0
        staged = root.host(root.canon(mk.D))
        for directory, directories, files in os.walk(staged):
            for path in [directory] + [os.path.join(directory, name) for name in directories + files]:
                value = os.lstat(path); key = (value.st_dev, value.st_ino)
                if key not in seen_inodes:
                    seen_inodes.add(key); allocated += value.st_blocks * 512
        with open(report, 'w') as stream:
            json.dump({'config_dir': confdir, 'arch': spec['arch'], 'kernel_release': release,
                       'uncompressed_size_mib': (allocated + (1 << 20) - 1) // (1 << 20),
                       'cpio_bytes': os.path.getsize(cpio_path), 'initrd_bytes': os.path.getsize(output),
                       'hooks_run': mk.hooks_run, 'orders': mk.orders, 'log': mk.log,
                       'busybox_applets': len(getattr(mk, 'busybox_applets', [])), 'entries': listing}, stream, indent=1)


if __name__ == '__main__':
    with open(sys.argv[1]) as stream:
        spec = json.load(stream)
    build_initrd(spec, sys.argv[2], sys.argv[3], sys.argv[4] if len(sys.argv) > 4 else None, sys.argv[5] if len(sys.argv) > 5 else None)
