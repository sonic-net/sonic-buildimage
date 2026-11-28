"""Store a dashboard snapshot as an index and immutable, byte-bounded run shards."""
import hashlib
import json
import os
from pathlib import Path
import re
import tempfile


SHARD_TARGET_BYTES = 8 * 1024**2
MAX_FILE_BYTES = 95 * 1024**2
SHARD_PATH = re.compile(r"history/([0-9a-f]{64})\.json")
INDEX_FIELDS = {"schema_version", "repository", "generated_at", "comparisons",
                "pull_requests", "run_shards"}


def _json_bytes(value):
    return json.dumps(value, sort_keys=True, separators=(",", ":"), allow_nan=False).encode("ascii")


def _replace(path, content):
    """Publish all bytes at once, including when another process reads the path."""
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = None
    try:
        with tempfile.NamedTemporaryFile(dir=path.parent, prefix=".history-", delete=False) as handle:
            temporary = Path(handle.name)
            handle.write(content)
            handle.flush()
            os.fsync(handle.fileno())
        os.replace(temporary, path)
    finally:
        if temporary is not None:
            temporary.unlink(missing_ok=True)


def _write_shard(directory, records):
    # Pack oldest-first so adding a new run leaves complete older shards reusable.
    # Reverse each batch to preserve the collector's newest-first order on disk.
    content = b"[" + b",".join(reversed(records)) + b"]\n"
    relative = f"history/{hashlib.sha256(content).hexdigest()}.json"
    path = directory / relative
    if (directory / "history").is_symlink() or path.is_symlink():
        raise ValueError("Refusing a symlink in the dashboard shard path")
    if not path.exists() or path.read_bytes() != content:
        _replace(path, content)
    return {"path": relative, "count": len(records)}


def write_snapshot(path, data):
    """Write all new shards before atomically replacing the entry point."""
    if data.get("schema_version") != 1 or not isinstance(data.get("runs"), list):
        raise ValueError("Expected a version 1 dashboard snapshot")
    parts = []
    records = []
    size = 3  # Opening bracket, closing bracket and newline.
    for run in reversed(data["runs"]):
        record = _json_bytes(run)
        if len(record) + 3 > MAX_FILE_BYTES:
            raise ValueError("A single dashboard run attempt exceeds the per-file publication limit")
        if records and size + 1 + len(record) > SHARD_TARGET_BYTES:
            parts.append(_write_shard(path.parent, records))
            records = []
            size = 3
        size += len(record) + bool(records)
        records.append(record)
    if records:
        parts.append(_write_shard(path.parent, records))
    index = {key: value for key, value in data.items() if key != "runs"}
    index.update(schema_version=2, run_shards=parts[::-1])
    content = _json_bytes(index) + b"\n"
    if len(content) > MAX_FILE_BYTES:
        raise ValueError("Dashboard index exceeds the per-file publication limit")
    _replace(path, content)


def read_snapshot(path):
    """Return one logical version 1 snapshot from either on-disk representation."""
    with path.open(encoding="utf-8") as handle:
        data = json.load(handle)
    if not isinstance(data, dict) or data.get("schema_version") != 2:
        return data  # The collector validates legacy and reconstructed metadata alike.
    if (set(data) - INDEX_FIELDS or not isinstance(data.get("run_shards"), list)
            or not isinstance(data.get("pull_requests"), list)
            or not isinstance(data.get("repository"), str)):
        raise ValueError("Invalid dashboard shard index")
    seen = set()
    runs = []
    for part in data.pop("run_shards"):
        if (not isinstance(part, dict) or set(part) != {"path", "count"}
                or type(part["count"]) is not int or part["count"] < 1
                or not isinstance(part["path"], str)):
            raise ValueError("Invalid dashboard shard descriptor")
        relative = part["path"]
        match = SHARD_PATH.fullmatch(relative)
        if not match or relative in seen:
            raise ValueError("Invalid or repeated dashboard shard path")
        seen.add(relative)
        shard = path.parent / relative
        if shard.parent.is_symlink() or shard.is_symlink() or shard.stat().st_size > MAX_FILE_BYTES:
            raise ValueError(f"Invalid dashboard shard file: {relative}")
        content = shard.read_bytes()
        if hashlib.sha256(content).hexdigest() != match.group(1):
            raise ValueError(f"Dashboard shard checksum mismatch: {relative}")
        records = json.loads(content)
        if not isinstance(records, list) or len(records) != part["count"]:
            raise ValueError(f"Dashboard shard record count mismatch: {relative}")
        runs.extend(records)
    data.update(schema_version=1, runs=runs)
    return data
