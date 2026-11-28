"""Reuse verified immutable inputs within one validation, never across builds.

Only content hashes and successful parsed inventories are retained. Reuse includes
checks made while scanning identical archive contents with the same parsing rules.
Overlay, receipt and ELF/debug checks still run for every consumer.
"""

from collections import OrderedDict
from contextlib import contextmanager
from contextvars import ContextVar
from copy import deepcopy
import hashlib
from pathlib import Path
import sys
import time

_current = ContextVar("validation_inputs", default=None)


def _stamp(path):
    value = path.stat()
    return (value.st_dev, value.st_ino, value.st_size, value.st_mtime_ns,
            value.st_ctime_ns, value.st_mode)


def _unchanged(path, before):
    if _stamp(path) != before:
        raise ValueError("validation input changed: " + str(path))


def _digest(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def _size(value):
    """Conservative retained-size bound for plain inventory containers."""
    result = sys.getsizeof(value)
    if isinstance(value, dict):
        result += sum(_size(key) + _size(item) for key, item in value.items())
    elif isinstance(value, (tuple, list)):
        result += sum(_size(item) for item in value)
    return result


class _Inputs:
    def __init__(self, paths, max_bytes, max_files):
        self.paths = [(Path(path).absolute(), Path(path).is_dir()) for path in paths]
        self.max_bytes, self.max_files = max_bytes, max_files
        self.files, self.inventories = {}, OrderedDict()
        self.bytes = 0
        self.counters = {
            "hash_reads": 0, "hash_hits": 0, "hashed_bytes": 0,
            "inventory_hits": 0, "inventory_misses": 0,
            "inventory_evictions": 0, "inventory_oversized": 0,
            "inventory_untracked": 0, "verification_reads": 0,
            "verification_bytes": 0, "verification_seconds": 0.0,
        }

    def contains(self, path):
        path = Path(path).absolute()
        return any(path == root or (directory and path.is_relative_to(root))
                   for root, directory in self.paths)

    def digest(self, path):
        path = Path(path).absolute()
        before = _stamp(path)
        if path in self.files:
            stamp, digest = self.files[path]
            _unchanged(path, stamp)
            self.counters["hash_hits"] += 1
            return digest
        digest = _digest(path)
        _unchanged(path, before)
        self.counters["hash_reads"] += 1
        self.counters["hashed_bytes"] += before[2]
        if len(self.files) < self.max_files:
            self.files[path] = (before, digest)
        return digest

    def inventory(self, path, kind, read):
        path = Path(path).absolute()
        before = _stamp(path)
        digest = self.digest(path)
        if path not in self.files:
            # Do not retain a result whose path cannot be checked at scope exit.
            self.counters["inventory_untracked"] += 1
            result = read()
            _unchanged(path, before)
            return result
        key = (kind, digest)
        if key in self.inventories:
            self.counters["inventory_hits"] += 1
            self.inventories.move_to_end(key)
            result = deepcopy(self.inventories[key][0])
        else:
            self.counters["inventory_misses"] += 1
            result = read()
            _unchanged(path, before)
            size = _size(result)
            if size <= self.max_bytes:
                while self.inventories and self.bytes + size > self.max_bytes:
                    _, (_, removed) = self.inventories.popitem(last=False)
                    self.bytes -= removed
                    self.counters["inventory_evictions"] += 1
                self.inventories[key] = (deepcopy(result), size)
                self.bytes += size
            else:
                self.counters["inventory_oversized"] += 1
        _unchanged(path, before)
        return result

    def verify(self):
        started = time.perf_counter()
        for path, (stamp, digest) in self.files.items():
            _unchanged(path, stamp)
            if _digest(path) != digest:
                raise ValueError("validation input changed: " + str(path))
            _unchanged(path, stamp)
            self.counters["verification_reads"] += 1
            self.counters["verification_bytes"] += stamp[2]
        self.counters["verification_seconds"] = time.perf_counter() - started

    def statistics(self):
        return {**self.counters, "tracked_files": len(self.files),
                "retained_inventories": len(self.inventories),
                "retained_inventory_bytes": self.bytes,
                "max_inventory_bytes": self.max_bytes, "max_files": self.max_files}


@contextmanager
def validation_input_cache(paths, *, max_bytes=64 * 1024 * 1024, max_files=512):
    """Share successful reads until scope exit; reject observed input mutation.

    Only the declared paths (and blobs below declared OCI directories) qualify;
    temporary extracted ELFs and other generated files are never retained.
    Each path is SHA256-verified before content-keyed reuse. Stat identities
    (including ctime, inode and device) are checked around reads and at hits;
    successful exit rechecks both identity and bytes before publishing results.
    These must be read-only validation inputs, not concurrent build outputs.
    Oversized inventories and paths beyond the file budget are
    read normally instead of retained.
    The yielded cache exposes statistics(); hash counters cover scoped reads,
    while verification counters measure the separate successful-exit recheck.
    """
    inputs = _Inputs(paths, max_bytes, max_files)
    token = _current.set(inputs)
    try:
        yield inputs
        inputs.verify()
    finally:
        _current.reset(token)


def sha256(path):
    inputs = _current.get()
    return inputs.digest(path) if inputs is not None and inputs.contains(path) else _digest(path)


def cached_inventory(path, kind, read):
    inputs = _current.get()
    return inputs.inventory(path, kind, read) if inputs is not None and inputs.contains(path) else read()
