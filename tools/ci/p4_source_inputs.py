#!/usr/bin/env python3
"""Fetch or verify the nine pinned public inputs for the native P4 recipes."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import stat
import subprocess
import time


BASE_URL = "https://downloadcontent.opensuse.org/repositories/home:/p4lang/Debian_11/"
VERSIONS = {"p4lang-pi": "0.1.3-2", "p4lang-bmv2": "1.15.0-9", "p4lang-p4c": "1.2.4.2-3"}
ALGORITHMS = {"md5": 32, "sha1": 40, "sha256": 64}


class InputError(Exception):
    """A public, fixed diagnostic code (never raw subprocess output)."""


def require(condition, code):
    if not condition:
        raise InputError(code)


def unique_pairs(pairs):
    result = {}
    for key, value in pairs:
        require(key not in result, "duplicate_json_key")
        result[key] = value
    return result


def fingerprints(path, expected_size=None, capture=False):
    """Read regular, stable files without following a final symlink."""
    fd = os.open(path, os.O_RDONLY | os.O_NOFOLLOW | os.O_NONBLOCK)
    with os.fdopen(fd, "rb") as stream:
        before = os.fstat(stream.fileno())
        require(stat.S_ISREG(before.st_mode), "nonregular_input")
        require(0 < before.st_size <= 512 * 1024 * 1024, "invalid_input_size")
        require(expected_size is None or before.st_size == expected_size, "input_size_mismatch:" + path.name)
        require(not capture or before.st_size <= 65536, "metadata_too_large")
        hashes = {name: hashlib.new(name, usedforsecurity=False) for name in ALGORITHMS}
        data = bytearray()
        count = 0
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            count += len(block)
            require(count <= before.st_size, "input_changed_during_read")
            for h in hashes.values():
                h.update(block)
            if capture:
                data.extend(block)
        after = os.fstat(stream.fileno())
    identity = lambda s: (s.st_dev, s.st_ino, s.st_size, s.st_mtime_ns, s.st_ctime_ns)
    require(identity(before) == identity(after) and count == before.st_size, "input_changed_during_read")
    return {"bytes": count, **{name: h.hexdigest() for name, h in hashes.items()}}, bytes(data)


def valid_fingerprint(value, algorithms):
    require(set(value) == {"bytes", *algorithms}, "invalid_fingerprint_fields")
    require(type(value["bytes"]) is int and 0 < value["bytes"] <= 512 * 1024 * 1024, "invalid_pinned_size")
    for name in algorithms:
        require(isinstance(value[name], str) and re.fullmatch(r"[0-9a-f]{%d}" % ALGORITHMS[name], value[name]), "invalid_pinned_hash")


def manifest_contract(manifest):
    require(manifest.get("schema") == 1 and manifest.get("base_url") == BASE_URL, "manifest_schema_or_origin")
    require(set(manifest["packages"]) == set(VERSIONS), "manifest_package_inventory")
    inventory = {}
    for package, version in VERSIONS.items():
        p = manifest["packages"][package]
        upstream = version.rsplit("-", 1)[0]
        require(p["version"] == version and p["upstream_version"] == upstream, "manifest_source_version")
        require(type(p["dsc_consistent"]) is bool, "invalid_dsc_consistency_flag")
        names = {f"{package}_{version}.dsc", f"{package}_{upstream}.orig.tar.gz", f"{package}_{version}.debian.tar.xz"}
        require(set(p["files"]) == names, "manifest_basename_inventory")
        for name, pin in p["files"].items():
            valid_fingerprint(pin, {"sha256"})
            inventory[name] = pin
    known = manifest["known_inconsistencies"]
    require(isinstance(known, list) and len(known) == 1, "known_inconsistency_inventory")
    k = known[0]
    require(k["package"] == "p4lang-p4c" and k["descriptor"] == "p4lang-p4c_1.2.4.2-3.dsc"
            and k["archive"] == "p4lang-p4c_1.2.4.2.orig.tar.gz", "known_inconsistency_identity")
    require(k["descriptor_sha256"] == inventory[k["descriptor"]]["sha256"], "known_descriptor_binding")
    for side in ["declared", "published"]:
        valid_fingerprint(k[side], set(ALGORITHMS))
    require(all(k["published"][field] == value for field, value in inventory[k["archive"]].items()), "known_archive_binding")
    require(k["declared"] != k["published"], "known_inconsistency_is_not_a_mismatch")
    return inventory


def descriptor_contract(data, package, version, archive_names):
    fields = {}
    current = None
    for line in data.decode("utf-8").splitlines():
        if line.startswith("-----BEGIN PGP SIGNATURE-----"):
            break
        if line.startswith((" ", "\t")) and current:
            fields[current] += "\n" + line.strip()
        elif re.match(r"^[A-Za-z][A-Za-z0-9-]*:", line):
            current, value = line.split(":", 1)
            require(current not in fields, "duplicate_descriptor_field")
            fields[current] = value.strip()
        else:
            current = None
    require(fields.get("Source") == package and fields.get("Version") == version
            and fields.get("Format") == "3.0 (quilt)", "descriptor_source_identity")
    declared = {name: {} for name in archive_names}
    for header, algorithm in [("Files", "md5"), ("Checksums-Sha1", "sha1"), ("Checksums-Sha256", "sha256")]:
        seen = set()
        for line in fields.get(header, "").splitlines():
            if not line.strip():
                continue
            parts = line.split()
            require(len(parts) == 3, "malformed_descriptor_checksum")
            checksum, size, name = parts
            require(name in archive_names and name not in seen, "descriptor_archive_inventory")
            require(re.fullmatch(r"[0-9a-f]{%d}" % ALGORITHMS[algorithm], checksum)
                    and re.fullmatch(r"[1-9][0-9]*", size), "malformed_descriptor_checksum")
            require("bytes" not in declared[name] or declared[name]["bytes"] == int(size), "descriptor_size_disagreement")
            declared[name].update(bytes=int(size), **{algorithm: checksum})
            seen.add(name)
        require(seen == archive_names, "missing_descriptor_checksums")
    return declared


def verify(directory, manifest, receipt):
    inventory = manifest_contract(manifest)
    require(directory.is_dir() and not directory.is_symlink(), "invalid_source_directory")
    require({p.name for p in directory.iterdir()} == set(inventory), "staged_file_inventory")
    descriptors = {}
    for name, pin in inventory.items():
        actual, data = fingerprints(directory / name, pin["bytes"], capture=name.endswith(".dsc"))
        require(actual["sha256"] == pin["sha256"], "input_sha256_mismatch:" + name)
        receipt["files"][name] = actual
        if name.endswith(".dsc"):
            descriptors[name] = data
    mismatches = []
    for package, p in manifest["packages"].items():
        dsc = f"{package}_{p['version']}.dsc"
        declared = descriptor_contract(descriptors[dsc], package, p["version"], set(p["files"]) - {dsc})
        local = []
        for name, expected in declared.items():
            actual = receipt["files"][name]
            if actual != expected:
                local.append({"package": package, "descriptor": dsc, "descriptor_sha256": receipt["files"][dsc]["sha256"],
                              "archive": name, "declared": expected, "published": actual})
        require(p["dsc_consistent"] == (not local), "descriptor_consistency_flag_mismatch")
        mismatches.extend(local)
        receipt["packages"][package] = {"version": p["version"], "dsc_consistent": not local}
    known = [{k: v for k, v in item.items() if k != "explanation"} for item in manifest["known_inconsistencies"]]
    require(mismatches == known, "unexpected_descriptor_inconsistency")
    receipt.update(known_inconsistencies=mismatches, all_published_pins_match=True,
                   all_dsc_checksums_match=not mismatches, descriptor_modified=False)


def fetch(directory, inventory, receipt, save):
    directory.mkdir(parents=True, exist_ok=False)
    deadline = time.monotonic() + 900
    for name, pin in inventory.items():
        attempts = receipt["transport"][name] = []
        file_deadline = min(deadline, time.monotonic() + 300)
        partial = directory / (name + ".partial")
        downloaded = False
        for attempt in range(1, 4):
            seconds = min(120, file_deadline - time.monotonic())
            require(seconds >= 1, "fetch_time_budget_exhausted")
            command = ["/usr/bin/curl", "--disable", "--silent", "--show-error", "--fail", "--location",
                       "--max-redirs", "3", "--proto", "=https", "--proto-redir", "=https", "--connect-timeout", "15",
                       "--max-time", str(seconds), "--max-filesize", str(pin["bytes"]), "--output", str(partial),
                       "--write-out", "%{http_code} %{size_download} %{time_total} %{num_redirects}", BASE_URL + name]
            try:
                proc = subprocess.run(command, capture_output=True, timeout=seconds + 2)
                metrics = proc.stdout.decode("ascii", errors="replace").split()
                attempts.append({"attempt": attempt, "curl_exit": proc.returncode,
                                 "metrics": dict(zip(["http_status", "bytes", "seconds", "redirects"], metrics)),
                                 "stderr_bytes": len(proc.stderr)})
                downloaded = proc.returncode == 0 and len(metrics) == 4 and metrics[0] == "200"
                retry = proc.returncode in {5, 6, 7, 18, 28, 35, 52, 55, 56} or (metrics and metrics[0] in {"408", "429", "500", "502", "503", "504"})
            except subprocess.TimeoutExpired:
                attempts.append({"attempt": attempt, "status": "process_timeout"})
                retry = True
            save()
            if downloaded:
                partial.rename(directory / name)
                break
            if not retry:
                break
            if attempt < 3 and file_deadline - time.monotonic() > 2:
                time.sleep(2)
        require(downloaded, "download_failed:" + name)


def run(manifest_path, directory, receipt_path, should_fetch=False):
    require(not receipt_path.resolve().is_relative_to(directory.resolve()), "receipt_must_be_outside_sources")
    receipt_path.parent.mkdir(parents=True, exist_ok=True)
    with receipt_path.open("x") as stream:
        receipt = {"schema": 1, "status": "running", "mode": "fetch" if should_fetch else "verify",
                   "files": {}, "packages": {}, "transport": {}, "openpgp_verified": False}

        def save():
            stream.seek(0)
            json.dump(receipt, stream, indent=2)
            stream.write("\n")
            stream.truncate()
            stream.flush()

        save()
        try:
            meta, raw = fingerprints(manifest_path, capture=True)
            receipt["manifest_sha256"] = meta["sha256"]
            manifest = json.loads(raw, object_pairs_hook=unique_pairs)
            inventory = manifest_contract(manifest)
            if should_fetch:
                fetch(directory, inventory, receipt, save)
            verify(directory, manifest, receipt)
            receipt["status"] = "verified_published_inputs"
        except (Exception, KeyboardInterrupt) as exc:
            receipt.update(status="failed", error=str(exc) if isinstance(exc, InputError) else type(exc).__name__)
        finally:
            save()
    return 0 if receipt["status"] == "verified_published_inputs" else 1


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path, required=True)
    parser.add_argument("--directory", type=Path, required=True)
    parser.add_argument("--receipt", type=Path, required=True)
    parser.add_argument("--fetch", action="store_true")
    args = parser.parse_args()
    raise SystemExit(run(args.manifest, args.directory, args.receipt, args.fetch))
