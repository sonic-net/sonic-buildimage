"""Summarize production artifact cache results without retaining spawn secrets."""

from collections import Counter
import json
import re

from tools.bazel.ci import public_artifacts
from tools.bazel.ci.artifact_validation import require

_CHUNK_SIZE = 64 * 1024
_MAX_RECORD_SIZE = 64 * 1024 * 1024


def _object(pairs):
    result = {}
    for key, value in pairs:
        require(key not in result, "duplicate execution-log field")
        result[key] = value
    return result


def _constant(_value):
    raise ValueError("invalid execution-log number")


def json_records(path):
    """Read consecutive JSON objects with bounded buffering and linear decode work."""
    decoder = json.JSONDecoder(object_pairs_hook=_object, parse_constant=_constant)
    with path.open() as stream:
        buffer, offset, done = "", 0, False
        while True:
            while offset < len(buffer) and buffer[offset].isspace():
                offset += 1
            if offset == len(buffer) and done:
                return
            if offset < len(buffer):
                try:
                    record, end = decoder.raw_decode(buffer, offset)
                except json.JSONDecodeError:
                    require(not done, "truncated or malformed execution log")
                else:
                    offset = end
                    yield record
                    continue
            # Large SpawnExec input lists must not be decoded again for every
            # 64 KiB. Double an incomplete record, keeping the same size limit.
            # An offset avoids repeatedly copying many small following records.
            buffer = buffer[offset:]
            offset = 0
            remaining = _MAX_RECORD_SIZE - len(buffer)
            require(remaining > 0, "execution-log record exceeds 64 MiB")
            chunk = stream.read(min(remaining, max(_CHUNK_SIZE, len(buffer))))
            done = not chunk
            buffer += chunk


def collect(execution_log, build_events, artifact_target):
    """Bind the complete successful artifact build to safe per-spawn counts."""
    public_artifacts.label(artifact_target)
    require(execution_log.is_file() and not execution_log.is_symlink(), "missing execution log")
    last = [None]
    with build_events.open() as stream:
        def lines():
            for line in stream:
                last[0] = line
                yield line
        summary = public_artifacts.bep_summary(lines())
    require(summary["command"] == "build" and
            summary["finished"] == {"success": True, "exit_code": 0} and
            summary["targets"].get(artifact_target) is True and
            last[0] is not None and public_artifacts.load_json(last[0]).get("lastMessage") is True,
            "incomplete production artifact build evidence")
    groups = Counter()
    for record in json_records(execution_log):
        require(isinstance(record, dict), "invalid execution-log record")
        target = record.get("targetLabel")
        if target == "":
            target = None
        if target is not None:
            public_artifacts.label(target)
        mnemonic, runner = record.get("mnemonic"), record.get("runner")
        require(isinstance(mnemonic, str) and re.fullmatch(r"[A-Za-z][A-Za-z0-9_]{0,79}", mnemonic),
                "invalid execution-log mnemonic")
        require(isinstance(runner, str) and re.fullmatch(r"[A-Za-z0-9_. -]{1,80}", runner),
                "invalid execution-log runner")
        cache_hit = record.get("cacheHit", False)
        require(isinstance(cache_hit, bool) and
                cache_hit == (runner in ("disk cache hit", "remote cache hit")),
                "invalid execution-log cache result")
        exit_code = record.get("exitCode", 0)
        require(record.get("status", "") == "" and type(exit_code) is int and exit_code == 0,
                "unsuccessful execution-log record")
        groups[(target, mnemonic, runner, cache_hit)] += 1
    require(bool(groups), "empty execution log cannot establish production cache reuse")
    actions = [{"target": target, "mnemonic": mnemonic, "runner": runner,
                "cache_hit": hit, "count": count}
               for (target, mnemonic, runner, hit), count in
               sorted(groups.items(), key=lambda item: (item[0][0] or "", *item[0][1:]))]
    action_count = sum(groups.values())
    hits = sum(item["count"] for item in actions if item["cache_hit"])
    return {"phase": "before_tests", "artifact_target": artifact_target,
            "action_count": action_count, "cache_hit_count": hits,
            "executed_count": action_count - hits, "actions": actions}
