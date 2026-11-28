"""Validate declared, already-built OCI pairs without running a build or a query."""

import hashlib
from email.parser import BytesParser
import json
import os
from pathlib import Path
import re
import tomllib
import zipfile

from diagnostics import ArtifactIdentifier, ArtifactType, ComparableArtifact, Modifier
from tools import Tool, Tools

# Intentionally empty until a comparison supplies evidence for a reviewed,
# exact external dependency difference. Entries belong in the PR introducing
# the difference, never in the caller's manifest. They do not accept OCI file
# or ELF diagnostics.
EXTERNAL_DEPENDENCY_DIFFERENCES = ()
# A source rule can explain build-metadata patch differences only when the
# original identity is unchanged and the retained prepared source trees match.
# Exclusions are exact, reviewed nonproduction paths, never manifest globs.
SOURCE_PATCH_DIFFERENCES = ()


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


def retained_file(record, base, inputs):
    require(isinstance(record, dict) and isinstance(record.get("origin"), str) and record["origin"], "missing retained input origin")
    path = checked_file(record, base)
    inputs.append(path)
    return record["sha256"]


def normalized_python_name(name):
    return re.sub(r"[-_.]+", "-", name).lower()


def retained_lock(record, base, inputs):
    """Read only supported original lock formats; a hash alone is not evidence."""
    retained_file(record, base, inputs)
    text = (base / record["path"]).read_text()
    kind = record.get("format")
    if kind == "cargo-lock":
        data = tomllib.loads(text).get("package")
        require(isinstance(data, list) and data, "missing Cargo.lock packages")
    elif kind == "apt-lock":
        data = json.loads(text).get("packages")
        require(isinstance(data, dict) and data, "missing apt.lock.json packages")
        data = list(data.values())
    elif kind == "debian-packages":
        data = []
        for paragraph in re.split(r"\n\s*\n", text.strip()):
            fields = BytesParser().parsebytes(paragraph.encode())
            data.append({k: fields.get(k) for k in ("Package", "Version", "Architecture", "SHA256")})
    elif kind in ("requirements-lock", "versions-py3"):
        data = []
        for line in re.sub(r"\\\r?\n", " ", text).splitlines():
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            match = re.fullmatch(r"([A-Za-z0-9][A-Za-z0-9_.-]*)==([^\s;*<>=!~]+)((?:\s+--hash=sha256:[0-9a-f]{64})*)", line)
            if match is None and kind == "requirements-lock":
                direct = re.fullmatch(r"([A-Za-z0-9][A-Za-z0-9_.-]*)\s+@\s+https://[^\s;]+((?:\s+--hash=sha256:[0-9a-f]{64})+)", line)
                if direct:
                    # SONiC source archives such as swsssdk belong in sources.
                    # The URL/hash does not attest a Python distribution version,
                    # so it cannot satisfy a versioned external dependency.
                    data.append({"name": normalized_python_name(direct[1]), "version": None,
                                 "hashes": re.findall(r"--hash=sha256:([0-9a-f]{64})", direct[2])})
                    continue
            require(match is not None, f"unsupported {kind} requirement: {line}")
            hashes = re.findall(r"--hash=sha256:([0-9a-f]{64})", match[3])
            require(bool(hashes) == (kind == "requirements-lock"), "wrong Python lock hash format")
            data.append({"name": normalized_python_name(match[1]), "version": match[2], "hashes": hashes})
    else:
        raise ValueError(f"unsupported retained lock format: {kind}")
    return kind, data


def verify_locked_dependency(name, dependency, lock, base, inputs):
    """Bind selected versions/hashes to original bytes, not caller annotations.

    Features and consumers are producer-recorded context: retain and check the
    exact selection receipt, while lock parsers independently verify the input
    identities. A selection receipt does not prove a complete build graph.
    """
    match = re.fullmatch(r"(apt|cargo|pip)/([^/]+)/([^/]+)", name)
    require(match is not None, "external dependency must name ecosystem/package/target")
    ecosystem, package, target = match.groups()
    kind, data = lock
    version = dependency["version"]
    matches = []
    if kind == "cargo-lock":
        require(ecosystem == "cargo", "Cargo lock used for non-Cargo input")
        matches = [p.get("checksum") for p in data if p.get("name") == package and p.get("version") == version
                   and isinstance(p.get("source"), str) and p["source"].startswith("registry+")]
    elif kind == "apt-lock":
        require(ecosystem == "apt", "APT lock used for non-APT input")
        matches = [p.get("sha256") for p in data if p.get("name") == package and p.get("version") == version
                   and p.get("architecture") in (target, "all")]
    elif kind == "debian-packages":
        require(ecosystem == "apt", "Packages index used for non-APT input")
        matches = [p["SHA256"] for p in data if p["Package"] == package and p["Version"] == version
                   and p["Architecture"] in (target, "all")]
    else:
        require(ecosystem == "pip" and package == normalized_python_name(package), "Python dependency name must be normalized")
        records = [p for p in data if p["name"] == package and p["version"] == version]
        require(len(records) == 1, f"dependency not uniquely present in retained lock: {name}")
        matches = records[0]["hashes"]
        if kind == "versions-py3":
            # Native Make constraints do not carry hashes. The actual consumed
            # wheel is required; neither its filename nor a synthetic hash is
            # sufficient to bind it to the pinned Python package.
            artifacts = dependency.get("artifacts")
            require(isinstance(artifacts, list) and artifacts, "versions-py3 requires retained wheels")
            matches = []
            for artifact in artifacts:
                checksum = retained_file(artifact, base, inputs)
                path = base / artifact["path"]
                require(path.suffix == ".whl", "only retained wheels support unhashed Python constraints")
                with zipfile.ZipFile(path) as wheel:
                    metadata = [n for n in wheel.namelist() if re.fullmatch(r"[^/]+\.dist-info/METADATA", n)]
                    require(len(metadata) == 1, "wheel must have one package metadata file")
                    fields = BytesParser().parsebytes(wheel.read(metadata[0]))
                require(len(fields.get_all("Name", [])) == 1 and len(fields.get_all("Version", [])) == 1,
                        "ambiguous wheel package metadata")
                require(normalized_python_name(fields["Name"]) == package and fields["Version"] == version,
                        "retained wheel does not match pinned package/version")
                matches.append(checksum)
            require(sorted(set(matches)) == dependency["checksums"], "selected wheel hashes differ from retained artifacts")
    require(matches and all(isinstance(h, str) and re.fullmatch(r"[0-9a-f]{64}", h) for h in matches),
            f"dependency has no verifiable checksum in retained lock: {name}")
    require(set(dependency["checksums"]) <= set(matches), f"selected checksum absent from retained lock: {name}")
    if kind != "versions-py3":
        require("artifacts" not in dependency, "unexpected artifacts for a hashed lock")
    evidence = dependency.get("selection_evidence")
    require(isinstance(evidence, list) and evidence, "missing producer selection evidence")
    selected = {k: dependency[k] for k in ("version", "checksums", "features", "images")}
    for record in evidence:
        retained_file(record, base, inputs)
        receipt = json.loads((base / record["path"]).read_text())
        require(isinstance(receipt, dict) and receipt.get("schema") == 1 and isinstance(receipt.get("dependencies"), dict),
                "invalid producer selection receipt")
        require(receipt["dependencies"].get(name) == selected, "producer selection receipt differs from claimed input")


def prepared_source(record, base, inputs):
    require(isinstance(record, dict) and set(record) == {"root", "manifest"}, "invalid prepared source record")
    require(isinstance(record["root"], str) and record["root"], "missing prepared source root")
    root = (base / record["root"]).absolute()
    require(root.is_dir(), "missing prepared source directory")
    inputs.append(root)
    retained_file(record["manifest"], base, inputs)
    manifest_path = base / record["manifest"]["path"]
    manifest = json.loads(manifest_path.read_text())
    require(set(manifest) == {"schema", "entries"} and manifest["schema"] == 1, "invalid prepared source manifest")
    entries = {}

    def visit(directory):
        for path in sorted(directory.iterdir()):
            name = path.relative_to(root).as_posix()
            if path.is_symlink():
                entries[name] = {"kind": "symlink", "target": os.readlink(path)}
                inputs.append(path)
            elif path.is_file():
                entries[name] = {"kind": "file", "sha256": digest(path), "mode": path.stat().st_mode & 0o7777}
                inputs.append(path)
            elif path.is_dir():
                entries[name] = {"kind": "directory", "mode": path.stat().st_mode & 0o7777}
                visit(path)
            else:
                raise ValueError(f"unsupported prepared source entry: {name}")

    visit(root)
    require(entries and manifest["entries"] == entries, "prepared source manifest differs from actual tree")
    return entries


def resolved_inputs(value, base, inputs, required):
    """Validate real source archives and resolved external inputs for schema 2.

    Lock formats differ between builders. Retain and verify each original lock,
    then compare its selected runtime dependencies, including features, target
    artifacts and consumers. Project source identity never uses this allowance.
    """
    sources = value.get("sources")
    require(isinstance(sources, dict) and sources, "missing complete source identity map")
    identities = {}
    prepared = {}
    for name, source in sources.items():
        require(isinstance(name, str) and name and isinstance(source, dict), "invalid source identity")
        kind = source.get("kind")
        if kind == "git":
            require(set(source) - {"prepared"} == {"kind", "revision", "patches"}, "invalid Git source fields")
            require(re.fullmatch(r"[0-9a-f]{40}", source.get("revision", "")), "missing source revision")
            identity = {"kind": kind, "revision": source["revision"]}
        elif kind == "archive":
            require(set(source) - {"prepared"} == {"kind", "release", "archives", "patches"}, "invalid archive source fields")
            require(isinstance(source.get("release"), str) and source["release"], "missing source release")
            archives = source.get("archives")
            require(isinstance(archives, dict) and archives, "missing source archives")
            require(all(isinstance(n, str) and n and Path(n).name == n for n in archives), "invalid source archive name")
            identity = {"kind": kind, "release": source["release"], "archives": {
                n: retained_file(record, base, inputs) for n, record in archives.items()
            }}
        else:
            raise ValueError("source kind must be git or archive")
        patches = source.get("patches")
        require(isinstance(patches, list), "missing ordered source patches")
        identity["patches"] = [retained_file(record, base, inputs) for record in patches]
        identities[name] = identity
        if "prepared" in source:
            prepared[name] = prepared_source(source["prepared"], base, inputs)

    locks = value.get("locks")
    require(isinstance(locks, dict) and locks, "missing dependency locks")
    parsed_locks = {}
    for name, record in locks.items():
        require(isinstance(name, str) and name, "invalid lock name")
        parsed_locks[name] = retained_lock(record, base, inputs)

    dependencies = value.get("external_dependencies")
    require(isinstance(dependencies, dict) and dependencies, "missing resolved external dependencies")
    resolved = {}
    used_locks = set()
    for name, dependency in dependencies.items():
        require(isinstance(name, str) and re.fullmatch(r"(?:apt|cargo|pip)/[^*?\[\]\s]+", name), "invalid external dependency name")
        require(isinstance(dependency, dict) and set(dependency) - {"artifacts"} == {"lock", "version", "checksums", "features", "images", "selection_evidence"}, "invalid resolved external dependency")
        require(isinstance(dependency["lock"], str) and dependency["lock"] in locks, "external dependency has no retained lock")
        require(isinstance(dependency["version"], str) and re.fullmatch(r"[^*?\[\]\s]+", dependency["version"]), "missing exact external version")
        checksums = dependency["checksums"]
        require(isinstance(checksums, list) and checksums and all(isinstance(x, str) and re.fullmatch(r"[0-9a-f]{64}", x) for x in checksums), "missing external artifact checksums")
        features = dependency["features"]
        require(isinstance(features, list) and all(isinstance(x, str) and x for x in features), "invalid external features")
        images = dependency["images"]
        require(isinstance(images, list) and images and all(isinstance(x, str) and x in required for x in images), "invalid external consumers")
        require(all(items == sorted(set(items)) for items in (checksums, features, images)), "resolved input lists must be sorted and unique")
        verify_locked_dependency(name, dependency, parsed_locks[dependency["lock"]], base, inputs)
        used_locks.add(dependency["lock"])
        resolved[name] = {k: dependency[k] for k in ("version", "checksums", "features", "images")}
    require(used_locks == set(locks), "retained lock has no resolved runtime dependencies")
    return identities, resolved, prepared


def compare_resolved_inputs(make, bazel):
    require(make[0].keys() == bazel[0].keys(), "Make/Bazel sources mismatch")
    accepted_sources = []
    for name, left in make[0].items():
        right = bazel[0][name]
        has_prepared = name in make[2] or name in bazel[2]
        if has_prepared:
            require(name in make[2] and name in bazel[2], "source comparison requires both prepared source trees")
        if left == right and (not has_prepared or make[2][name] == bazel[2][name]):
            continue
        origin = {k: v for k, v in left.items() if k != "patches"}
        require(origin == {k: v for k, v in right.items() if k != "patches"}, "Make/Bazel sources mismatch")
        matches = [rule for rule in SOURCE_PATCH_DIFFERENCES
                   if rule.get("source") == name and rule.get("identity") == origin
                   and rule.get("make_patches") == left["patches"] and rule.get("bazel_patches") == right["patches"]
                   and isinstance(rule.get("id"), str) and rule["id"]
                   and isinstance(rule.get("reason"), str) and rule["reason"]]
        require(len(matches) == 1, f"Make/Bazel sources mismatch: unaccepted patches for {name}")
        rule = matches[0]
        exclusions = rule.get("nonproduction_paths")
        require(isinstance(exclusions, list) and all(isinstance(p, str) and p and not p.startswith("/")
                and ".." not in p.split("/") and not any(c in p for c in "*?[]") for p in exclusions), "invalid source path exclusions")
        require(len(set(exclusions)) == len(exclusions), "duplicate source path exclusions")
        require(name in make[2] and name in bazel[2], "patch difference requires both prepared source trees")
        left_tree, right_tree = make[2][name], bazel[2][name]
        require(set(exclusions) <= left_tree.keys() | right_tree.keys(), "stale source path exclusion")
        require({p: v for p, v in left_tree.items() if p not in exclusions} ==
                {p: v for p, v in right_tree.items() if p not in exclusions}, "prepared production sources mismatch")
        accepted_sources.append(rule)
    require(make[1].keys() == bazel[1].keys(), "Make/Bazel external dependency set mismatch")
    accepted = []
    for name, left in make[1].items():
        right = bazel[1][name]
        if left == right:
            continue
        matches = [rule for rule in EXTERNAL_DEPENDENCY_DIFFERENCES
                   if rule.get("dependency") == name and rule.get("make") == left and rule.get("bazel") == right
                   and isinstance(rule.get("id"), str) and rule["id"]
                   and isinstance(rule.get("reason"), str) and rule["reason"]]
        require(len(matches) == 1, f"unaccepted external dependency difference: {name}")
        accepted.append(matches[0])
    return {"accepted_external_dependencies": accepted, "accepted_source_patches": accepted_sources}


def provenance(record, base, builder, revision, *, schema=1):
    path = checked_file(record, base)
    value = json.loads(path.read_text())
    require(value.get("schema") == schema and value.get("builder") == builder, "wrong provenance schema/builder")
    require(value.get("buildimage_commit") == revision and value.get("clean") is True, "source checkout mismatch or dirty build")
    require(isinstance(value.get("command"), list) and value["command"] and all(isinstance(x, str) and x for x in value["command"]), "missing original build command")
    config = value.get("configuration", {})
    require(all(isinstance(config.get(k), str) and config[k] for k in ("distribution", "architecture", "platform")), "missing build configuration")
    require(isinstance(config.get("features"), dict), "missing feature selection")
    if schema == 1:
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


def load(path, revision, *, report=None, provenance_report=None):
    """Return exact OCI sources, required names, tools and the parsed manifest hash."""
    require(re.fullmatch(r"[0-9a-f]{40}", revision), "invalid checkout revision")
    manifest_bytes = path.read_bytes()
    manifest = json.loads(manifest_bytes)
    manifest_sha256 = hashlib.sha256(manifest_bytes).hexdigest()
    inputs = [path]
    schema = manifest.get("schema")
    require(type(schema) is int and schema in (1, 2), "unsupported manifest schema")
    images = manifest.get("required_images")
    require(isinstance(images, list) and images and len(images) == len(set(images)), "missing or duplicate required images")
    require(all(isinstance(n, str) and re.fullmatch(r"docker-[a-z0-9][a-z0-9-]*", n) and not n.endswith("-dbg") for n in images), "invalid required runtime image name")
    required = {n + suffix for n in images for suffix in (".gz", "-dbg.gz")}
    base = path.parent
    make = provenance(manifest.get("make_provenance"), base, "make", revision, schema=schema)
    bazel = provenance(manifest.get("bazel_provenance"), base, "bazel", revision, schema=schema)
    inputs.extend(base / manifest[builder + "_provenance"]["path"] for builder in ("make", "bazel"))
    for key in (("configuration", "sources", "locks") if schema == 1 else ("configuration",)):
        require(make[key] == bazel[key], f"Make/Bazel {key} mismatch")
    if schema == 2:
        accepted = compare_resolved_inputs(
            resolved_inputs(make, base, inputs, required),
            resolved_inputs(bazel, base, inputs, required),
        )
        if provenance_report is not None:
            provenance_report.update(make=make, bazel=bazel, **accepted)
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
            if input_path.is_dir():
                aliases = aliases or input_path.resolve() in report.resolve().parents
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
