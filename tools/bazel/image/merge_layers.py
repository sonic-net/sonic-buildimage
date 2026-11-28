#!/usr/bin/env python3
"""Merge rootfs layer tars (OCI semantics) and squash the result.

Replaces "extract every layer as the build user, then mksquashfs -all-root":
the layers are merged as tar streams, so numeric uid/gid, setuid/setgid/sticky
bits, device nodes and SCHILY.xattr records survive exactly as the layers carry
them, and upper layers can delete lower content with OCI whiteouts.

Merge semantics (layers applied in command-line order):
  * Paths are normalized (leading "./" and "/" dropped); "." is the root.
  * Intermediate path components are resolved in the merged tree, following
    symlinks chroot-style (absolute targets from the image root, ".." clamped
    at the root). This is GNU tar --keep-directory-symlink / dpkg behaviour and
    implements merged-/usr: with merged_usr_skeleton first (bin -> usr/bin),
    ./bin/x lands in usr/bin/x. Missing parents are created 0755 0/0 mtime 0.
  * A directory entry over a directory merges (metadata of the last entry
    wins); over a symlink that resolves to a directory the symlink is kept;
    over anything else it replaces it.
  * A non-directory entry replaces whatever is at the path (a directory is
    removed with its whole subtree).
  * Hardlinks are bound to the inode their target has at that point in the
    stream. Later overrides/whiteouts of either name do not affect the other;
    when emitting, the first surviving name (in output order) carries the data
    and the others link to it, so a link whose target was removed, replaced or
    excluded is materialized instead of dangling.
  * Whiteouts (applied to the lower layers only, before the layer's own
    entries): "dir/.wh.<name>" removes dir/<name> (file, symlink or subtree);
    "dir/.wh..wh..opq" removes every lower-layer child of dir. Other
    ".wh..wh.*" entries (aufs metadata) are dropped. No whiteout entry is
    emitted.
  * --exclude patterns use mksquashfs -wildcards semantics (per-component
    glob anchored at the root, "..." prefix = any depth); excluded subtrees
    are dropped before hardlinks are resolved.

The merged tree is written as a deterministic PAX tar (depth-first, children
in byte order, numeric owners only, no uname/gname so the host passwd/group
databases are never consulted) straight into `mksquashfs - <out> -tar`.
"""
import argparse
import bz2
import fnmatch
import gzip
import io
import lzma
import os
import shutil
import subprocess
import sys
import tarfile
import tempfile

ENC, ERR = "utf-8", "surrogateescape"
WH = ".wh."
OPAQUE = ".wh..wh..opq"
MAX_SYMLINK_HOPS = 40


class MergeError(Exception):
    pass


class Meta:
    __slots__ = ("mode", "uid", "gid", "mtime", "xattrs")

    def __init__(self, mode=0o755, uid=0, gid=0, mtime=0, xattrs=None):
        self.mode, self.uid, self.gid, self.mtime = mode, uid, gid, mtime
        self.xattrs = xattrs or {}

    @classmethod
    def of(cls, ti):
        return cls(ti.mode & 0o7777, ti.uid, ti.gid, max(int(ti.mtime), 0),
                   {k: v for k, v in ti.pax_headers.items() if k.startswith("SCHILY.xattr.")})


class Dir:
    __slots__ = ("meta", "children")

    def __init__(self, meta=None):
        self.meta = meta or Meta()
        self.children = {}


class Inode:
    """Any non-directory; shared by every hardlinked name."""
    __slots__ = ("meta", "type", "linkname", "devmajor", "devminor", "size", "src")

    def __init__(self, ti, src):
        self.meta = Meta.of(ti)
        self.type = ti.type
        self.linkname = ti.linkname if ti.type == tarfile.SYMTYPE else ""
        self.devmajor, self.devminor = ti.devmajor, ti.devminor
        self.size = ti.size if ti.isreg() else 0
        self.src = src  # (TarFile, TarInfo) or bytes, regular files only

    def is_symlink(self):
        return self.type == tarfile.SYMTYPE


REG_TYPES = (tarfile.REGTYPE, tarfile.AREGTYPE, tarfile.CONTTYPE)
NONDIR_TYPES = REG_TYPES + (tarfile.SYMTYPE, tarfile.CHRTYPE, tarfile.BLKTYPE, tarfile.FIFOTYPE)


def split_path(name):
    parts = [p for p in name.split("/") if p not in ("", ".")]
    if ".." in parts:
        raise MergeError("refusing tar member with '..': %r" % name)
    return parts


class Tree:
    def __init__(self):
        self.root = Dir()

    def resolve_dir(self, parts, create):
        """Resolve directory path `parts`, following symlinks chroot-style.

        Returns the Dir, or None (create=False) when it does not exist or is
        not a directory. With create=True missing real directories are made,
        but a symlink must already resolve to a directory (GNU tar / dpkg fail
        rather than create directories behind a dangling link).
        """
        r = self._resolve(parts, create, 0)
        return None if r is None else r[0]

    def _resolve(self, parts, create, hops):
        canon = []          # canonical (symlink-free) path of `chain[-1]`
        chain = [self.root]
        pending = list(parts)
        while pending:
            c = pending.pop(0)
            if c in ("", "."):
                continue
            if c == "..":
                if canon:
                    canon.pop()
                    chain.pop()
                continue
            cur = chain[-1]
            child = cur.children.get(c)
            if child is None:
                if not create:
                    return None
                child = cur.children[c] = Dir()
            if isinstance(child, Dir):
                canon.append(c)
                chain.append(child)
                continue
            if child.is_symlink():
                hops += 1
                if hops > MAX_SYMLINK_HOPS:
                    raise MergeError("too many levels of symlinks resolving %s" % "/".join(parts))
                t = child.linkname
                base = [] if t.startswith("/") else canon
                r = self._resolve(base + t.split("/"), False, hops)
                if r is None:
                    if create:
                        raise MergeError("%s: symlink %s -> %s does not resolve to a directory"
                                         % ("/".join(parts), "/".join(canon + [c]), t))
                    return None
                canon = list(r[1])
                chain = [self.root]
                for n in canon:
                    chain.append(chain[-1].children[n])
                continue
            if create:
                raise MergeError("%s: path component %r is not a directory" % ("/".join(parts), c))
            return None
        return chain[-1], canon

    def lookup(self, parts):
        """Entry at `parts` without following a final symlink (or None)."""
        if not parts:
            return self.root
        parent = self.resolve_dir(parts[:-1], create=False)
        return None if parent is None else parent.children.get(parts[-1])


def parent_for_create(tree, parts, what):
    try:
        return tree.resolve_dir(parts[:-1], create=True)
    except MergeError as e:
        raise MergeError("%s: %s" % (what, e))


def open_layer(path, scratch):
    """Open a layer as a seekable, uncompressed TarFile (decompressing to scratch)."""
    with open(path, "rb") as f:
        magic = f.read(6)
    opener = None
    if magic[:2] == b"\x1f\x8b":
        opener = gzip.open
    elif magic[:3] == b"BZh":
        opener = bz2.open
    elif magic[:6] == b"\xfd7zXZ\x00":
        opener = lzma.open
    if opener is not None:
        fd, plain = tempfile.mkstemp(suffix=".tar", dir=scratch)
        with os.fdopen(fd, "wb") as out, opener(path, "rb") as src:
            shutil.copyfileobj(src, out, 1 << 20)
        path = plain
    return tarfile.open(path, "r:", encoding=ENC, errors=ERR)


def apply_layer(tree, tf, label):
    members = tf.getmembers()
    # Pass 1: whiteouts, against the lower layers only.
    for ti in members:
        parts = split_path(ti.name)
        if not parts or not parts[-1].startswith(WH):
            continue
        parent = tree.resolve_dir(parts[:-1], create=False)
        if parent is None:
            continue
        base = parts[-1]
        if base == OPAQUE:
            parent.children.clear()
        elif not base.startswith(WH + WH):
            parent.children.pop(base[len(WH):], None)
    # Pass 2: the layer's own entries, in stream order.
    for ti in members:
        parts = split_path(ti.name)
        if parts and parts[-1].startswith(WH):
            continue
        where = "%s: %s" % (label, ti.name)
        if ti.isdir():
            if not parts:
                tree.root.meta = Meta.of(ti)
                continue
            parent = parent_for_create(tree, parts, where)
            cur = parent.children.get(parts[-1])
            if isinstance(cur, Dir):
                cur.meta = Meta.of(ti)
            elif isinstance(cur, Inode) and cur.is_symlink() and \
                    tree.resolve_dir(parts, create=False) is not None:
                pass  # --keep-directory-symlink: keep e.g. bin -> usr/bin
            else:
                parent.children[parts[-1]] = Dir(Meta.of(ti))
            continue
        if not parts:
            raise MergeError("%s: non-directory root entry" % where)
        if ti.islnk():
            tparts = split_path(ti.linkname)
            target = tree.lookup(tparts)
            if not isinstance(target, Inode):
                raise MergeError("%s: hardlink target %r %s" % (
                    where, ti.linkname, "is a directory" if target else "does not exist"))
            node = target
        elif ti.type in NONDIR_TYPES:
            node = Inode(ti, (tf, ti) if ti.isreg() else None)
        else:
            raise MergeError("%s: unsupported tar member type %r" % (where, ti.type))
        parent = parent_for_create(tree, parts, where)
        parent.children[parts[-1]] = node


def compile_excludes(patterns):
    out = []
    for p in patterns:
        anywhere = p.startswith("...")
        comps = split_path(p[3:] if anywhere else p)
        if not comps:
            raise MergeError("empty exclude pattern %r" % p)
        out.append((anywhere, comps))
    return out


def _comp_match(pat, name):
    # FNM_PERIOD: a leading '.' must be matched explicitly.
    if name.startswith(".") and not pat.startswith("."):
        return False
    return fnmatch.fnmatchcase(name, pat)


def excluded(path, excludes):
    for anywhere, comps in excludes:
        starts = range(len(path) - len(comps) + 1) if anywhere else [0]
        for s in starts:
            seg = path[s:s + len(comps)]
            if len(seg) == len(comps) and s + len(comps) == len(path) and \
                    all(_comp_match(p, n) for p, n in zip(comps, seg)):
                return True
    return False


def _key(name):
    return name.encode(ENC, ERR)


def emit(tree, out, excludes):
    first_name = {}  # id(Inode) -> output path of the data-carrying name

    def header(name, meta, typ):
        ti = tarfile.TarInfo(name)
        ti.type = typ
        ti.mode, ti.uid, ti.gid, ti.mtime = meta.mode, meta.uid, meta.gid, meta.mtime
        ti.uname = ti.gname = ""
        ti.pax_headers = dict(sorted(meta.xattrs.items()))
        return ti

    def walk(d, prefix):
        for name in sorted(d.children, key=_key):
            path = prefix + [name]
            if excludes and excluded(path, excludes):
                continue
            node = d.children[name]
            pstr = "/".join(path)
            if isinstance(node, Dir):
                out.addfile(header(pstr, node.meta, tarfile.DIRTYPE))
                walk(node, path)
                continue
            if id(node) in first_name:
                ti = header(pstr, node.meta, tarfile.LNKTYPE)
                ti.linkname = first_name[id(node)]
                out.addfile(ti)
                continue
            first_name[id(node)] = pstr
            ti = header(pstr, node.meta, tarfile.REGTYPE if node.type in REG_TYPES else node.type)
            ti.linkname = node.linkname
            ti.devmajor, ti.devminor = node.devmajor, node.devminor
            if ti.type == tarfile.REGTYPE:
                ti.size = node.size
                if isinstance(node.src, bytes):
                    out.addfile(ti, io.BytesIO(node.src))
                else:
                    src_tf, src_ti = node.src
                    out.addfile(ti, src_tf.extractfile(src_ti))
            else:
                out.addfile(ti)

    walk(tree.root, [])


def ensure_default_file(tree, path, content):
    parts = split_path(path)
    if tree.lookup(parts) is not None:
        return
    ti = tarfile.TarInfo(path)
    ti.mode, ti.size = 0o644, len(content)
    parent = tree.resolve_dir(parts[:-1], create=True)
    parent.children[parts[-1]] = Inode(ti, content)


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--mksquashfs", required=True, help="hermetic mksquashfs binary")
    ap.add_argument("--out", required=True, help="output squashfs image")
    ap.add_argument("--exclude", action="append", default=[],
                    help="mksquashfs -wildcards exclude pattern (repeatable)")
    ap.add_argument("--default-file", action="append", default=[], metavar="PATH=CONTENT",
                    help="add PATH (0644 0/0) with CONTENT+newline if absent after the merge")
    ap.add_argument("--mksquashfs-arg", action="append", default=[],
                    help="extra mksquashfs argument (repeatable)")
    ap.add_argument("layers", nargs="*", help="layer tars (.tar/.tar.gz), lowest first")
    args = ap.parse_args(argv)

    excludes = compile_excludes(args.exclude)
    tree = Tree()
    scratch = tempfile.mkdtemp(prefix="merge_layers.")
    opened = []
    try:
        for layer in args.layers:
            tf = open_layer(layer, scratch)
            opened.append(tf)
            apply_layer(tree, tf, layer)
        for spec in args.default_file:
            path, _, content = spec.partition("=")
            ensure_default_file(tree, path, (content + "\n").encode())

        root = tree.root.meta
        # -exit-on-error: never silently drop an entry mksquashfs can't add.
        # Root inode metadata comes from the layers' "./" entry (default
        # 0755 0/0 mtime 0) instead of mksquashfs's build-user/now defaults.
        cmd = [args.mksquashfs, "-", args.out, "-tar", "-noappend", "-exit-on-error",
               "-root-mode", "%04o" % root.mode, "-root-uid", str(root.uid),
               "-root-gid", str(root.gid), "-root-time", str(root.mtime)]
        if "SOURCE_DATE_EPOCH" not in os.environ:  # both together are refused
            cmd += ["-mkfs-time", "0"]
        cmd += args.mksquashfs_arg
        if args.exclude:
            # Already applied above (so hardlinks into excluded paths get
            # materialized); passed on unchanged as the authoritative filter.
            # "-e" consumes every remaining argument, so it goes last, once.
            cmd += ["-wildcards", "-e"] + args.exclude
        proc = subprocess.Popen(cmd, stdin=subprocess.PIPE)
        try:
            with tarfile.open(fileobj=proc.stdin, mode="w|", format=tarfile.PAX_FORMAT,
                              encoding=ENC, errors=ERR, copybufsize=1 << 20) as out:
                emit(tree, out, excludes)
        except BrokenPipeError:
            pass
        finally:
            try:
                proc.stdin.close()
            except BrokenPipeError:
                pass
        rc = proc.wait()
        if rc:
            raise MergeError("mksquashfs failed with exit code %d" % rc)
    finally:
        for tf in opened:
            tf.close()
        shutil.rmtree(scratch, ignore_errors=True)


if __name__ == "__main__":
    try:
        main()
    except MergeError as e:
        sys.exit("merge_layers: error: %s" % e)
