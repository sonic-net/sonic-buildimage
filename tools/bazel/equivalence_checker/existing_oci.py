"""Validate declared, already-built OCI pairs without running a build or a query."""

import hashlib
import json
import os
from pathlib import Path
import re

from diagnostics import ArtifactIdentifier, ArtifactType, ComparableArtifact, Modifier
from tools import Tool, Tools


def require(condition, message):
    if not condition:
        raise ValueError(message)


def digest(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def checked_file(record, base, *, allow_missing=False):
    require(isinstance(record, dict), "file record must be an object")
    require(isinstance(record.get("path"), str) and record["path"], "missing file path")
    path = (base / record["path"]).absolute()
    require(re.fullmatch(r"[0-9a-f]{64}", record.get("sha256", "")), "missing SHA256")
    if allow_missing and not path.exists():
        return path
    require(path.is_file(), f"missing file: {path}")
    require(digest(path) == record["sha256"], f"SHA256 mismatch: {path}")
    return path


def provenance(record, base, builder, revision):
    path = checked_file(record, base)
    value = json.loads(path.read_text())
    require(value.get("schema") == 1 and value.get("builder") == builder, "wrong provenance schema/builder")
    require(value.get("buildimage_commit") == revision and value.get("clean") is True, "source checkout mismatch or dirty build")
    require(isinstance(value.get("command"), list) and value["command"] and all(isinstance(x, str) and x for x in value["command"]), "missing original build command")
    config = value.get("configuration", {})
    require(all(isinstance(config.get(k), str) and config[k] for k in ("distribution", "architecture", "platform")), "missing build configuration")
    require(isinstance(config.get("features"), dict), "missing feature selection")
    sources = value.get("sources", {})
    require(isinstance(sources, dict) and sources, "missing complete source identity map")
    for name, identity in sources.items():
        require(isinstance(name, str) and name and isinstance(identity, dict), "invalid source identity")
        require(re.fullmatch(r"[0-9a-f]{40}", identity.get("revision", "")), "missing source revision")
        require(isinstance(identity.get("patches"), list) and all(isinstance(p, str) and re.fullmatch(r"[0-9a-f]{64}", p) for p in identity["patches"]), "missing ordered source patch hashes")
    locks = value.get("locks", {})
    require(isinstance(locks, dict) and locks and all(isinstance(k, str) and k and isinstance(v, str) and re.fullmatch(r"[0-9a-f]{64}", v) for k, v in locks.items()), "missing dependency lock identities")
    require(isinstance(value.get("artifacts"), dict) and value["artifacts"], "missing producer artifact hashes")
    if builder == "make":
        require(value.get("BUILD_WITH_BAZEL_WHEN_AVAILABLE") == "n", "Make provenance must disable Bazel")
    return value


def load(path, revision, *, report=None):
    """Return exact OCI sources, required names, tools and the parsed manifest hash."""
    require(re.fullmatch(r"[0-9a-f]{40}", revision), "invalid checkout revision")
    manifest_bytes = path.read_bytes()
    manifest = json.loads(manifest_bytes)
    manifest_sha256 = hashlib.sha256(manifest_bytes).hexdigest()
    inputs = [path]
    require(manifest.get("schema") == 1, "unsupported manifest schema")
    images = manifest.get("required_images")
    require(isinstance(images, list) and images and len(images) == len(set(images)), "missing or duplicate required images")
    require(all(isinstance(n, str) and re.fullmatch(r"docker-[a-z0-9][a-z0-9-]*", n) and not n.endswith("-dbg") for n in images), "invalid required runtime image name")
    required = {n + suffix for n in images for suffix in (".gz", "-dbg.gz")}
    base = path.parent
    make = provenance(manifest.get("make_provenance"), base, "make", revision)
    bazel = provenance(manifest.get("bazel_provenance"), base, "bazel", revision)
    inputs.extend(base / manifest[builder + "_provenance"]["path"] for builder in ("make", "bazel"))
    for key in ("configuration", "sources", "locks"):
        require(make[key] == bazel[key], f"Make/Bazel {key} mismatch")
    pairs = manifest.get("pairs")
    require(isinstance(pairs, list) and pairs, "missing OCI pairs")
    artifacts = []
    names = set()
    for pair in pairs:
        name = pair.get("name", "")
        require(isinstance(name, str) and re.fullmatch(r"docker-[a-z0-9][a-z0-9-]*\.gz", name), "only named OCI .gz pairs are supported")
        require(name not in names, "duplicate OCI pair")
        names.add(name)
        paths = {}
        for side, identity in (("make", make), ("bazel", bazel)):
            record = pair.get(side)
            paths[side] = checked_file(record, base, allow_missing=name not in required)
            inputs.append(paths[side])
            require(paths[side].name == name, f"wrong {side} archive filename")
            require(identity["artifacts"].get(name) == record["sha256"], f"{side} producer hash mismatch")
        if paths["make"].exists() and paths["bazel"].exists():
            require(not os.path.samefile(paths["make"], paths["bazel"]), "Make and Bazel must not be the same artifact file")
        identifier = ArtifactIdentifier(name, None, frozenset({Modifier.DEBUG if name.endswith("-dbg.gz") else Modifier.RUNTIME}))
        artifacts.append(ComparableArtifact(identifier, paths["bazel"], paths["make"], ArtifactType.OCI_IMAGE))
    require(required <= names, "missing required runtime/debug OCI pair")
    tool_records = manifest.get("tools", {})
    require(set(tool_records) == {"readelf", "abidiff", "elfcompare"}, "declare only the three OCI comparison tools")
    tools = {}
    for name, record in tool_records.items():
        require(isinstance(record.get("origin"), str) and record["origin"], f"missing {name} tool origin")
        executable = checked_file(record, base)
        inputs.append(executable)
        closure = record.get("closure")
        require(isinstance(closure, list) and closure, f"missing {name} tool closure")
        for item in closure:
            inputs.append(checked_file(item, base))
        # Preserve the invocation path: resolving symlinks can break Bazel runfiles.
        tools[name] = Tool(name, (str(executable),))
    if report is not None:
        for input_path in inputs:
            aliases = report.resolve() == input_path.resolve()
            if report.exists() and input_path.exists():
                aliases = aliases or os.path.samefile(report, input_path)
            require(not aliases, f"report path aliases an input: {input_path}")
    return artifacts, required, Tools(**tools, dpkg_deb=None), manifest_sha256


def root_name(identifier):
    while identifier.source is not None:
        identifier = identifier.source
    return identifier.name


def scoped_result(classified, required, compared):
    """Keep every diagnostic; only required pairs determine the scoped gate."""
    missing = sorted(required - set(compared))
    unaccepted = classified.get(None, [])
    failed = sorted({root_name(d.artifact) for d in unaccepted if root_name(d.artifact) in required})
    # An exclusion or incomplete comparison cannot make a required pair pass,
    # even if a future acceptance rule classifies that diagnostic as accepted.
    incomplete = {"EXCLUDED_BY_TAG", "MODULE_UNREACHABLE", "NO_MAKE_ARTIFACT", "NO_BAZEL_ARTIFACT", "UNREADABLE", "INCOMPLETE", "UNPARSEABLE", "ERROR"}
    failed = sorted(set(failed) | {root_name(d.artifact) for group in classified.values() for d in group if str(d.code) in incomplete and root_name(d.artifact) in required})
    return {"required_pairs": sorted(required), "compared_pairs": sorted(compared), "missing_pairs": missing, "failed_required_pairs": failed, "required_status": "FAIL" if missing or failed else "PASS", "all_compared_status": "FAIL" if unaccepted else "PASS", "diagnostic_pairs": sorted(set(compared) - required)}
