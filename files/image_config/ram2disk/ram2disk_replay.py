#!/usr/bin/env python3
"""Replay tmpfs log rotations on the disk mirror to prevent rsync overwriting/retransfers.

Usage: ram2disk_replay.py {replay|commit|recover-staging} SOURCE MIRROR STATE_DIR STAGING_DIR
ram2disk.sh holds the shared rotation/sync lock and calls:
  replay           before rsync: finish pending work, then replay new rotations.
  commit           after successful rsync: remove stale map records and raw
                   history copies whose matching archives are now on disk.
  recover-staging  before archiving: finish pending work or keep orphaned files.

Renaming the disk copies first lets rsync reuse their bytes instead of copying
each rotated log again. Files are tracked by inode, not logrotate naming rules.


State, in STATE_DIR (tmpfs, kept for one boot):
  inode-map        saved records, sorted by inode:
                     <ino> <size> <mtime_ns> <head> <tail> <path>
                   Head/tail hashes describe the start and end of the recorded
                   prefix; matching them does not compare every byte.
  inode-map.next   replacement map written during replay or commit.
  source-scan      temporary source listing, sorted by inode.
  replay-plan      R/D/H operations, with the header indicating the current phase and progress count.
  drop-after-sync  raw history copies waiting for their matching archive on disk.
STAGING_DIR is on the mirror's filesystem and temporarily holds files being moved.


Planning: compare inode-map with a new scan, both sorted by inode.
  Prefix checks use head/tail hashes, not a full byte-by-byte comparison.

  1. Same inode still exists:
     unchanged or only appended, same path -> leave to rsync (send new bytes)
     unchanged or only appended, new path  -> R: rename the disk copy first
     compressed file                      -> assume unchanged; R if renamed
     old prefix no longer matches         -> look for the old data in step 3

  2. Inode is gone:
     recorded as empty                    -> D: delete if disk copy is empty;
                                               otherwise keep it as history
     compressed file or no saved hashes   -> H: keep as history
     any other file                       -> look for the old data in step 3

  3. Resolve superseded disk copies (e.g. copytruncate, rename-and-compress),
     checking for the old data in this order:
     new uncompressed file matches prefix -> R: rename there; rsync sends the rest
     same-path file looks edited in place -> leave to rsync (update in place)
     new archive matches old prefix       -> D: move raw copy to history now;
                                               delete after rsync copies archive
     none of the above                    -> H: keep as <name>~<mtime>

     For the in-place edit check: a smaller file means truncation. If it did not
     shrink, one matching hash suggests an edit (e.g. lastlog or journal files).
     If both hashes differ, new files in/below its directory suggest truncation
     followed by new writes; otherwise treat it as an in-place edit.

  New source files are copied by rsync; no disk rename is needed.

Applying: phase 1 renames R sources directly when the target is free, or
stages them when it is occupied (H and D act at once). A progress counter
prevents repeating operations whose source paths were subsequently refilled.
Phase 2 places the staged files. Errors abort and resume on the next call.
"""
# Imports are kept small on purpose: hashlib loads OpenSSL, and argparse,
# subprocess, shutil and json each add RSS; the interpreter alone is ~12 MB.
import errno, os, stat, sys, time
try:
    from _blake2 import blake2b
except ImportError:
    from hashlib import blake2b

HEAD_BYTES = 512                 # fingerprint window at the start of a file
TAIL_BYTES = 4096                # fingerprint window ending at the recorded size
HASH_BYTES = 8
READ_CHUNK = 64 * 1024
SORT_MEMORY = '1M'               # GNU sort buffer; bigger inputs spill to STATE_DIR
MAX_CANDIDATES = 10000          # new files remembered per replay (bounds memory)
MAX_MISSING_SOURCES = 10000      # beyond this, every R operation goes through staging
NO_HASH = b'-'
COMPRESSION_FORMATS = {
    b'.gz': 'gzip', b'.xz': 'lzma', b'.bz2': 'bz2',
    b'.zst': None, b'.zip': None,
}
HISTORY_SEP = b'~'
HISTORY_TIME = '%Y%m%dT%H%M%S'   # UTC
RECOVERED_DIR = b'ram2disk-recovered'
OPS = (b'R', b'D', b'H')
# Classification reasons for resolving a previously mapped mirror copy:
GONE, TRUNCATED, SUSPECTED_REGROWN, REWRITTEN = (
    'gone', 'truncated', 'suspected-regrown', 'rewritten')
OP_NAMES = {b'R': 'renamed', b'D': 'dropped (archived)', b'H': 'kept as history'}


def log(msg):
    print('ram2disk-replay: ' + msg, file=sys.stderr)


def digest(data):
    return blake2b(data, digest_size=HASH_BYTES).hexdigest().encode()


def is_compressed(path):
    return os.path.splitext(path)[1] in COMPRESSION_FORMATS


def split_compression_ext(name):
    for ext in COMPRESSION_FORMATS:
        if name.endswith(ext) and len(name) > len(ext):
            return name[:-len(ext)], ext
    return name, b''


def fingerprint(path, size):
    """Hash windows at the start and end of the first `size` bytes; '-' on failure."""
    head_len, tail_start = min(HEAD_BYTES, size), max(0, size - TAIL_BYTES)
    try:
        fd = os.open(path, os.O_RDONLY | os.O_NOFOLLOW)
    except OSError:
        return NO_HASH, NO_HASH
    try:
        if tail_start == 0:                          # one read covers both windows
            tail = os.pread(fd, size, 0)
            head = tail[:head_len]
        else:
            head = os.pread(fd, head_len, 0)
            tail = os.pread(fd, size - tail_start, tail_start)
    except OSError:
        return NO_HASH, NO_HASH
    finally:
        os.close(fd)
    if len(head) != head_len or len(tail) != size - tail_start:
        return NO_HASH, NO_HASH                      # file is shorter than `size` now
    return digest(head), digest(tail)


def file_key(st):
    return b'%016x' % st.st_ino


def read_records(path):
    """NUL-terminated records of a file, read with a fixed-size buffer."""
    with open(path, 'rb') as f:
        rest = b''
        while True:
            chunk = f.read(READ_CHUNK)
            if not chunk:
                break
            *records, rest = (rest + chunk).split(b'\0')
            yield from records
    if rest:
        yield rest


def unlink_if_exists(path):
    try:
        os.unlink(path)
    except FileNotFoundError:
        pass


def start_sort(dst, tmp_dir):
    """Pipe the scan into GNU sort instead of storing an unsorted copy."""
    env = dict(os.environ, LC_ALL='C')
    argv = ['sort', '-z', '-S', SORT_MEMORY, '-T', os.fsdecode(tmp_dir), '-o', os.fsdecode(dst)]
    reader, writer = os.pipe()
    try:
        pid = os.posix_spawnp(argv[0], argv, env, file_actions=[
            (os.POSIX_SPAWN_DUP2, reader, 0)])
    except BaseException:
        os.close(writer)
        raise
    finally:
        os.close(reader)
    return pid, os.fdopen(writer, 'wb')


def wait_sort(pid):
    rc = os.waitstatus_to_exitcode(os.waitpid(pid, 0)[1])
    if rc != 0:
        raise OSError(errno.EIO, 'sort exited %d' % rc)


class Paths:
    def __init__(self, source, mirror, state_dir, staging):
        self.source, self.mirror, self.staging = source, mirror, staging
        self.state_dir = state_dir
        in_state = lambda name: os.path.join(state_dir, name)
        self.inode_map = in_state(b'inode-map')
        self.next_map = in_state(b'inode-map.next')
        self.plan = in_state(b'replay-plan')
        self.drop_list = in_state(b'drop-after-sync')
        self.scan = in_state(b'source-scan')


class Entry:
    """One inode-map record: the mirror copy at `path` starts with the first
    `size` bytes of tmpfs inode `key`, fingerprinted by `head` and `tail`."""
    __slots__ = ('key', 'size', 'mtime', 'head', 'tail', 'path')

    def __init__(self, key, size, mtime, head, tail, path):
        self.key, self.size, self.mtime = key, size, mtime
        self.head, self.tail, self.path = head, tail, path

    @classmethod
    def from_map(cls, record):
        key, size, mtime, head, tail, path = record.split(b'\t', 5)
        return cls(key, int(size), int(mtime), head, tail, path)

    @classmethod
    def from_scan(cls, record):
        key, size, mtime, path = record.split(b'\t', 3)
        return cls(key, int(size), int(mtime), NO_HASH, NO_HASH, path)

    def encode(self):
        return b'%s\t%d\t%d\t%s\t%s\t%s\0' % (self.key, self.size, self.mtime,
                                             self.head, self.tail, self.path)


# --------------------------------------------------------------------- scanning
def scan_source(paths):
    """Write the sorted scan: "<ino> <size> <mtime_ns> <path>" for every
    regular single-link file under SOURCE. Files with several hard links are
    left to rsync alone: one inode with two names cannot be replayed."""
    pid, out = start_sort(paths.scan, paths.state_dir)
    try:
        with out:
            pending = [b'']
            while pending:
                rel_dir = pending.pop()
                try:
                    entries = os.scandir(os.path.join(paths.source, rel_dir))
                except (FileNotFoundError, NotADirectoryError):
                    continue                         # removed while we were scanning
                with entries:
                    for entry in entries:
                        rel = os.path.join(rel_dir, entry.name) if rel_dir else entry.name
                        try:
                            st = entry.stat(follow_symlinks=False)
                        except FileNotFoundError:
                            continue
                        if stat.S_ISDIR(st.st_mode):
                            pending.append(rel)
                        elif stat.S_ISREG(st.st_mode) and st.st_nlink == 1:
                            out.write(b'%s\t%d\t%d\t%s\0' % (file_key(st), st.st_size,
                                                            st.st_mtime_ns, rel))
    finally:
        wait_sort(pid)


# --------------------------------------------------------------------- planning
class Planner:
    """Turns inode-level changes into mirror operations (see module docstring)."""

    def __init__(self, source, plan_file, track_new_files):
        self.source, self.plan_file = source, plan_file
        self.track_new_files = track_new_files   # False on a first sync: nothing to match
        self.counts = dict.fromkeys(OPS, 0)
        self.rewritten_in_place = 0
        self.superseded = []          # (entry, reason for no longer matching)
        self.new_raw_by_head = {}     # head hash -> [[size, path], ...]  new uncompressed files
        self.new_archives = []        # new compressed files
        self.new_archive_by_head = None   # built lazily, only if something needs it
        self.remembered = 0
        self.dirs_with_new_files = set()   # directories at or above any new file

    def emit(self, op, path, target=b''):
        self.plan_file.write(op + b'\0' + path + b'\0' + target + b'\0')
        self.counts[op] += 1

    @property
    def op_count(self):
        return sum(self.counts.values())

    def live(self, rel):
        return os.path.join(self.source, rel)

    # --- one call per inode of the merge join ---------------------------------
    def same_inode(self, old, new):
        renamed = old.path != new.path
        if is_compressed(new.path):                    # archives never change content, they're considered immutable
            if renamed:
                self.emit(b'R', old.path, new.path)
            return
        if new.size == old.size and new.mtime == old.mtime:
            new.head, new.tail = old.head, old.tail
            return self.emit(b'R', old.path, new.path) if renamed else None
        if old.size == 0:
            head = tail = b''
        elif old.head == NO_HASH or new.size < old.size:
            head = tail = None
        else:
            head, tail = fingerprint(self.live(new.path), old.size)
        new.head, new.tail = fingerprint(self.live(new.path), new.size)
        if old.size == 0 or (head, tail) == (old.head, old.tail):     # appended only
            if renamed:
                self.emit(b'R', old.path, new.path)
        elif renamed:                    # renamed and rewritten: treat as gone + new
            self.superseded.append((old, GONE))
            self.remember_new_file(new)
        elif new.size < old.size:        # truncated: copytruncate, or rotate 0
            self.superseded.append((old, TRUNCATED))
        elif head != old.head and tail != old.tail:
            self.superseded.append((old, SUSPECTED_REGROWN))  # truncated, then regrown past the old size?
        else:
            self.superseded.append((old, REWRITTEN))   # edited in place (lastlog, journal)

    def vanished(self, old):
        if old.size == 0:
            self.emit(b'D', old.path)
        elif is_compressed(old.path) or old.head == NO_HASH:
            self.emit(b'H', old.path)    # an archive is never recompressed: tmpfs deletes it, so preserve history on disk
        else:
            self.superseded.append((old, GONE))

    def created(self, new):
        if not is_compressed(new.path):
            new.head, new.tail = fingerprint(self.live(new.path), new.size)
        self.remember_new_file(new)

    def remember_new_file(self, new):
        if not self.track_new_files or self.remembered >= MAX_CANDIDATES:
            return
        self.remembered += 1
        parent = os.path.dirname(new.path)
        while parent not in self.dirs_with_new_files:  # also its ancestors (olddir is often
            self.dirs_with_new_files.add(parent)       # a subdirectory of the log's directory)
            if not parent:
                break
            parent = os.path.dirname(parent)
        if is_compressed(new.path):
            self.new_archives.append((new.key, new.path))
        elif new.head != NO_HASH:
            self.new_raw_by_head.setdefault(new.head, []).append([new.size, new.path])

    # --- superseded mirror copies, resolved by content -------------------------
    def resolve_superseded(self):
        for old, how in self.superseded:
            if how == SUSPECTED_REGROWN:
                # Both windows differ: infer truncation/regrowth if a tracked new
                # file appeared in/below this directory; otherwise its inplace rewrite.
                how = TRUNCATED if os.path.dirname(old.path) in self.dirs_with_new_files else REWRITTEN
            copy = self.claim_uncompressed_copy(old)
            if copy is not None:
                if copy != old.path:
                    self.emit(b'R', old.path, copy)
            elif how == REWRITTEN:
                self.rewritten_in_place += 1
            else:
                archive = self.find_archive(old)
                if archive is not None:
                    self.emit(b'D', old.path, archive)
                else:
                    self.emit(b'H', old.path)

    def claim_uncompressed_copy(self, old):
        """A new uncompressed file that starts with the bytes the mirror holds
        (copytruncate's copy, a rename+rewrite, ...). Each is claimed once."""
        bucket = self.new_raw_by_head.get(old.head)
        for i, (size, path) in enumerate(bucket or ()):
            if size >= old.size and fingerprint(self.live(path), old.size)[1] == old.tail:
                del bucket[i]
                return path
        return None

    def find_archive(self, old):
        """Return a new archive's inode/path if its decompressed head/tail match.
        Read archive heads once; check prefix tails only for matching heads."""
        if self.new_archive_by_head is None:
            self.new_archive_by_head = {}
            for key, path in self.new_archives:
                head = archive_head(self.live(path))
                if head is not None:
                    self.new_archive_by_head.setdefault(head, []).append((key, path))
        for key, path in self.new_archive_by_head.get(old.head, ()):
            if archive_has_prefix(self.live(path), old):
                return key + b'\t' + path          # D's target: which archive, by inode
        return None


def open_archive(path):
    module = COMPRESSION_FORMATS.get(os.path.splitext(path)[1])
    return __import__(module).open(path, 'rb') if module else None   # imported lazily


def archive_head(path):
    try:
        archive = open_archive(path)
        if archive is None:
            return None                 # unsupported codec: not a match candidate
        with archive:
            return digest(archive.read(HEAD_BYTES))
    except Exception:                   # corrupt or truncated archive
        return None


def archive_has_prefix(path, old):
    """Streams the archive up to old.size; memory stays at READ_CHUNK."""
    try:
        if path.endswith(b'.gz') and old.size < 2 ** 32:
            with open(path, 'rb') as f:            # gzip trailer: uncompressed size mod 2^32
                f.seek(-4, os.SEEK_END)
                if int.from_bytes(f.read(4), 'little') < old.size:
                    return False
        with open_archive(path) as archive:
            tail_start = max(0, old.size - TAIL_BYTES)
            to_skip = tail_start
            while to_skip:
                got = len(archive.read(min(READ_CHUNK, to_skip)))
                if not got:
                    return False
                to_skip -= got
            tail = archive.read(old.size - tail_start)
            return len(tail) == old.size - tail_start and digest(tail) == old.tail
    except Exception:
        return False


def plan_replay(paths):
    """Write the next map and publish a plan only if operations are needed.
    Return the Planner for operation counts."""
    scan_source(paths)
    have_map = os.path.exists(paths.inode_map)
    old_entries = map(Entry.from_map, read_records(paths.inode_map)) if have_map else iter(())
    new_entries = map(Entry.from_scan, read_records(paths.scan))
    with open(paths.next_map, 'wb') as next_map, open(paths.plan, 'wb') as plan_file:
        plan_file.write(plan_header(PLAN_WRITING, 0))
        planner = Planner(paths.source, plan_file, have_map)
        old, new, last_key = next(old_entries, None), next(new_entries, None), None
        while old is not None or new is not None:
            if new is not None and new.key == last_key:
                new = next(new_entries, None)        # seen twice: renamed during the scan
                continue
            if new is None or (old is not None and old.key < new.key):
                planner.vanished(old)
                old = next(old_entries, None)
                continue
            if old is None or new.key < old.key:
                planner.created(new)
            else:
                planner.same_inode(old, new)
                old = next(old_entries, None)
            next_map.write(new.encode())
            last_key, new = new.key, next(new_entries, None)
        planner.resolve_superseded()
    os.unlink(paths.scan)
    if planner.op_count:
        fd = os.open(paths.plan, os.O_WRONLY)
        try:
            set_plan_state(fd, PLAN_PHASE1, 0)       # commit point: planning is complete
        finally:
            os.close(fd)
    else:
        os.unlink(paths.plan)
    return planner


PLAN_WRITING, PLAN_PHASE1, PLAN_PHASE2 = b'0', b'1', b'2'


def plan_header(state, done):
    return b'%s%012d\0' % (state, done)


def set_plan_state(fd, state, done):
    header = plan_header(state, done)
    if os.pwrite(fd, header, 0) != len(header):
        raise OSError(errno.EIO, 'short replay progress write')


def read_plan_state(paths):
    try:
        with open(paths.plan, 'rb') as f:
            header = f.read(14)
    except FileNotFoundError:
        return None, 0
    if header[:2] in (b'R\0', b'M\0', b'D\0', b'H\0'):
        # Upgrade an interrupted plan from the separate-marker format.
        marker, progress = paths.plan + b'.staged', paths.plan + b'.progress'
        state, done = PLAN_PHASE1, 0
        if os.path.exists(marker):
            state = PLAN_PHASE2
        else:
            try:
                with open(progress, 'rb') as f:
                    data = f.read()
            except FileNotFoundError:
                data = b''
            if data:
                if len(data) != 12 or not data.isdigit():
                    raise OSError(errno.EIO, 'invalid replay progress', os.fsdecode(progress))
                done = int(data)
        tmp = paths.plan + b'.tmp'
        with open(paths.plan, 'rb') as src, open(tmp, 'wb') as dst:
            dst.write(plan_header(state, done))
            while True:
                chunk = src.read(READ_CHUNK)
                if not chunk:
                    break
                dst.write(chunk)
        os.replace(tmp, paths.plan)
        unlink_if_exists(marker)
        unlink_if_exists(progress)
        return state, done
    if not header or header[:1] == PLAN_WRITING:
        return PLAN_WRITING, 0              # planning never reached its commit point
    if (len(header) != 14 or header[:1] not in (PLAN_PHASE1, PLAN_PHASE2)
            or header[-1:] != b'\0' or not header[1:13].isdigit()):
        raise OSError(errno.EIO, 'invalid replay progress header', os.fsdecode(paths.plan))
    return header[:1], int(header[1:13])


# --------------------------------------------------------------------- applying
def history_name(path):
    """<dir>/<stem>~<mtime>[<compression ext>], made unique: the name under
    which the mirror keeps a generation that tmpfs no longer has."""
    stamp = time.strftime(HISTORY_TIME, time.gmtime(os.lstat(path).st_mtime)).encode()
    parent, name = os.path.split(path)
    stem, ext = split_compression_ext(name)
    candidate, n = os.path.join(parent, stem + HISTORY_SEP + stamp + ext), 0
    while os.path.lexists(candidate):
        n += 1
        candidate = os.path.join(parent, b'%s%s%s-%d%s' % (stem, HISTORY_SEP, stamp, n, ext))
    return candidate


def keep_as_history(path):
    os.rename(path, history_name(path))


def drop_if_empty(src):
    """The mirror file may contain bytes written after the source scan; if it's empty, delete it, otherwise preserve as history."""
    try:
        size = os.lstat(src).st_size
    except FileNotFoundError:
        return
    if size == 0:
        os.unlink(src)
    else:
        keep_as_history(src)


def park_until_archived(paths, src, archive):
    """Keep the raw copy as history until rsync has written the archive that
    holds the same bytes: if rsync keeps failing and logrotate deletes that
    archive meanwhile, the history copy is the only one left.
    `archive` is "<ino>\t<path>" of that archive on tmpfs."""
    kept = history_name(src)
    os.rename(src, kept)
    with open(paths.drop_list, 'ab') as f:
        f.write(kept + b'\0' + archive + b'\0')


def drop_archived_copies(paths):
    """Delete parked raw copies whose archive is now on disk, complete."""
    if not os.path.exists(paths.drop_list):
        return
    records = read_records(paths.drop_list)
    for kept in records:
        key, archive = next(records).split(b'\t', 1)
        try:
            live = os.lstat(os.path.join(paths.source, archive))
            done = (file_key(live) == key and   # still that archive, and rsync copied it whole
                    os.lstat(os.path.join(paths.mirror, archive)).st_size == live.st_size)
        except OSError:
            done = False                    # archive gone or not on disk: keep the history copy
        if done:
            unlink_if_exists(kept)
    os.unlink(paths.drop_list)


def read_plan(plan):
    records = read_records(plan)
    next(records, None)                     # fixed-width state/progress header
    for n, op in enumerate(records):
        if op == b'M':                      # resume plans written before M merged into R
            op = b'R'
        yield n, op, next(records), next(records)


def guarded(op, path, action):
    """Abort on any operation error, preserving the plan for the next sync."""
    try:
        action()
    except OSError as err:
        log('operation %s %s failed: %s; aborting replay, plan retained for retry '
            'on next sync; persistent failures require intervention'
            % (op.decode(), os.fsdecode(path), err))
        raise


def recover_orphaned_staging(paths):
    """Staged files without a plan (the plan was lost, e.g. reboot mid-replay):
    their names are unknown, so keep them in the mirror under one directory."""
    try:
        if not os.listdir(paths.staging):
            return
    except FileNotFoundError:
        return
    stamp = time.strftime(HISTORY_TIME, time.gmtime()).encode()
    target = os.path.join(paths.mirror, RECOVERED_DIR + HISTORY_SEP + stamp)
    os.makedirs(paths.mirror, exist_ok=True)
    os.rename(paths.staging, target)
    log('replay plan lost; kept staged files without their names in %s' % os.fsdecode(target))


APPLIED = {'direct': 0, 'staged': 0}


def apply_plan(paths):
    """Apply or resume replay-plan, then promote inode-map.next."""
    state, done = read_plan_state(paths)
    if state not in (PLAN_PHASE1, PLAN_PHASE2):
        if state is not None:
            os.unlink(paths.plan)
        recover_orphaned_staging(paths)
        return
    mirror, staging = paths.mirror, paths.staging
    mirror_prefix = mirror.rstrip(b'/') + b'/'
    on_mirror = lambda rel: mirror_prefix + rel
    # Staged as <n>~<original name>: unique per operation, and readable if the
    # plan is ever lost (see recover_orphaned_staging).
    staged = lambda n, path: os.path.join(
        staging, b'%d%s%s' % (n, HISTORY_SEP, os.path.basename(path)[:200]))
    made_dirs = set()

    def make_parent(dst):
        parent = os.path.dirname(dst)
        if parent not in made_dirs:
            os.makedirs(parent, exist_ok=True)
            made_dirs.add(parent)

    if state == PLAN_PHASE1:
        os.makedirs(staging, exist_ok=True)
        # Never fill a missing source belonging to a later operation: it would
        # then move the wrong generation. Bound memory by falling back to staging.
        missing = set()
        for n, op, path, target in read_plan(paths.plan):
            if n >= done and not os.path.lexists(on_mirror(path)):
                missing.add(path)
                if len(missing) > MAX_MISSING_SOURCES:
                    missing = None
                    break
        fd = os.open(paths.plan, os.O_WRONLY)
        try:
            for n, op, path, target in read_plan(paths.plan):
                if n < done:
                    continue
                src = on_mirror(path)
                src_exists = (path not in missing if missing is not None
                              else os.path.lexists(src))
                if op == b'D' and not target:
                    guarded(op, src, lambda: drop_if_empty(src))
                elif not src_exists:
                    pass
                elif op == b'D':
                    guarded(op, src, lambda: park_until_archived(paths, src, target))
                elif op == b'H':
                    guarded(op, src, lambda: keep_as_history(src))
                elif (missing is not None and target not in missing
                      and not os.path.lexists(on_mirror(target))):
                    dst = on_mirror(target)

                    def direct():
                        make_parent(dst)
                        os.rename(src, dst)
                        APPLIED['direct'] += 1
                    guarded(op, dst, direct)
                elif not os.path.lexists(staged(n, path)):
                    def stage():
                        os.rename(src, staged(n, path))
                        APPLIED['staged'] += 1
                    guarded(op, src, stage)
                # The phase byte stays unchanged while advancing the count.
                if os.pwrite(fd, b'%012d' % (n + 1), 1) != 12:
                    raise OSError(errno.EIO, 'short replay progress write')
            set_plan_state(fd, PLAN_PHASE2, 0)
        finally:
            os.close(fd)

    # Phase 2 only fills mirror paths, from staging; a staged file is moved once.
    try:
        in_staging = set(os.listdir(staging))
    except FileNotFoundError:
        in_staging = set()
    for n, op, path, target in (read_plan(paths.plan) if in_staging else ()):
        if op != b'R' or os.path.basename(staged(n, path)) not in in_staging:
            continue
        dst, src = on_mirror(target), staged(n, path)

        def place():
            make_parent(dst)
            if os.path.lexists(dst):
                keep_as_history(dst)        # an unmapped file holds the target: keep it
            os.rename(src, dst)
        guarded(op, dst, place)

    if os.path.exists(paths.next_map):
        os.replace(paths.next_map, paths.inode_map)
    os.unlink(paths.plan)
    recover_orphaned_staging(paths)


# --------------------------------------------------------------------- commands
def replay(paths):
    os.makedirs(paths.state_dir, exist_ok=True)
    os.makedirs(paths.mirror, exist_ok=True)
    apply_plan(paths)                       # an interrupted replay is finished first
    planner = plan_replay(paths)
    if planner.op_count:
        apply_plan(paths)
        done = ['%d %s' % (planner.counts[op], OP_NAMES[op]) for op in OPS if planner.counts[op]]
        if APPLIED['staged']:
            done.append('%d of the renames via staging' % APPLIED['staged'])
        if planner.rewritten_in_place:
            done.append('%d left to rsync (edited in place)' % planner.rewritten_in_place)
        print('replayed: ' + ', '.join(done))
    else:
        os.replace(paths.next_map, paths.inode_map)


def commit(paths):
    """After successful rsync keep only the inode-map records that are still
    true: same inode at that path, still starting with the fingerprinted bytes.
    Records for files changed during rsync are dropped; that can only cost
    bytes on a later sync, never data loss."""
    if os.path.exists(paths.plan):
        raise OSError(errno.EBUSY, 'a replay plan is pending', os.fsdecode(paths.plan))
    drop_archived_copies(paths)
    if not os.path.exists(paths.inode_map):
        return
    tmp = paths.next_map
    with open(tmp, 'wb') as out:
        for record in read_records(paths.inode_map):
            entry = Entry.from_map(record)
            live = os.path.join(paths.source, entry.path)
            try:
                st = os.lstat(live)
            except OSError:
                continue
            if file_key(st) != entry.key or st.st_nlink != 1:
                continue
            if (st.st_size, st.st_mtime_ns) != (entry.size, entry.mtime):
                if is_compressed(entry.path) or st.st_size < entry.size:
                    continue
                if entry.size and fingerprint(live, entry.size) != (entry.head, entry.tail):
                    continue
            out.write(record + b'\0')
    os.replace(tmp, paths.inode_map)


COMMANDS = {'replay': replay, 'commit': commit, 'recover-staging': apply_plan}


def main(argv):
    if len(argv) != 6 or argv[1] not in COMMANDS:
        print('usage: %s {replay|commit|recover-staging} SOURCE MIRROR STATE_DIR STAGING_DIR'
              % argv[0],
              file=sys.stderr)
        return 2
    paths = Paths(*map(os.fsencode, argv[2:]))
    try:
        COMMANDS[argv[1]](paths)
    except OSError as err:
        log('%s failed: %s' % (argv[1], err))
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
