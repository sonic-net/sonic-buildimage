#!/usr/bin/env python3
"""Check that orchagent APT assembly preserves locked content and base ELF files."""

import ast
import hashlib
import io
import json
from pathlib import Path
import struct
import sys
import tarfile
import tempfile
import unittest
from unittest import mock

GENERATED_INPUTS = Path(sys.argv.pop(1))
POLICY = Path(sys.argv.pop(1))
LOCK = Path(sys.argv.pop(1))
IMPORTED_PACKAGES = {
    ("orchagent_debian", "arping"): (Path(sys.argv.pop(1)), Path(sys.argv.pop(1))),
    ("orchagent_debug_debian", "gdbserver"): (Path(sys.argv.pop(1)), Path(sys.argv.pop(1))),
}
from sonic_apt.dependencies import package_from_control
from tools.bazel.tests.oci_base_fixture import digest, oci_files, layer_tar, write_layout
from tools.bazel.oci import apt_selection as subject
from tools.bazel.oci.oci_inventory import assert_overlay_paths


def tar_bytes(entries):
    return layer_tar(dict(entries))


def control_tar(fields):
    text = "".join(name + ": " + value + "\n" for name, value in fields.items())
    return tar_bytes([("control", text.encode())])


def write_oci(path, entries):
    layer = tar_bytes(entries)
    config = {"architecture": "amd64", "os": "linux",
              "rootfs": {"type": "layers", "diff_ids": [digest(layer)]}}
    write_layout(path, oci_files(json.dumps(config).encode(), [layer]))


class SelectAptPayloadsTest(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory(prefix="orchagent-apt-test-")
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        header = bytearray(64)
        header[:6] = b"\x7fELF\x02\x01"
        struct.pack_into("<HH", header, 16, 3, 62)
        self.base_elf = bytes(header) + b"FIPS runtime"
        self.status = ("Package: libssl3t64\nVersion: 3.5.7+fips\nArchitecture: amd64\n"
                       "Status: install ok installed\n\n").encode()
        self.base = self.root / "base.oci"
        self.write_base()
        packages, roots, mapping = {}, [], []
        self.paths, self.controls, self.control_fields = {}, {}, {}
        for name, version in (("libssl3t64", "3.5.6"), ("new-runtime", "1.0")):
            key = "/trixie/" + name + ":amd64=" + version
            path = self.root / (name + ".tar")
            data = ([("usr/lib/libssl.so.3", bytes(header) + b"Debian runtime")] if name == "libssl3t64" else
                    [("usr/share/" + name + "/data", name.encode())])
            path.write_bytes(tar_bytes(data))
            control = self.root / (name + ".control.tar")
            fields = {"Package": name, "Version": version, "Architecture": "amd64"}
            if name == "new-runtime":
                fields["Depends"] = "libssl3t64 (>= 3.5)"
            control.write_bytes(control_tar(fields))
            self.control_fields[name] = fields
            packages[key] = {"name": name, "version": version, "architecture": "amd64", "suite": "trixie",
                             "sha256": hashlib.sha256(name.encode()).hexdigest(), "size": 1,
                             "filename": "pool/" + name + ".deb", "depends_on": [],
                             "payload_sha256": hashlib.sha256(path.read_bytes()).hexdigest(), "payload_size": path.stat().st_size,
                             "control_sha256": hashlib.sha256(control.read_bytes()).hexdigest(), "control_size": control.stat().st_size}
            roots.append(key)
            mapping.append({"key": key, "package": packages[key], "payload": str(path), "control": str(control)})
            self.paths[name], self.controls[name] = path, control
        self.lock = self.root / "apt.lock.json"
        self.lock.write_text(json.dumps({"version": 2, "facts": {},
            "dependency_sets": {group: {"sets": {"amd64": dict(key.rsplit("=", 1) for key in roots)}}
                                for group in ("runtime", "debug")},
            "sources": {"trixie": {"uris": ["https://snapshot.debian.org/archive/debian/20260727T143429Z"]}},
            "packages": packages}))
        self.mapping = self.root / "mapping.json"
        self.mapping.write_text(json.dumps({"architecture": "amd64", "locked": mapping}))
        self.policy = self.root / "policy.json"
        self.policy.write_bytes(POLICY.read_bytes())

    def write_base(self, *, status=True):
        entries = [("usr/lib/libssl.so.3", self.base_elf), ("etc/base", b"base config")]
        if status:
            entries.append(("var/lib/dpkg/status", self.status))
        write_oci(self.base, entries)

    def update_payload(self, name, entries):
        path = self.paths[name]
        path.write_bytes(tar_bytes(entries))
        lock = json.loads(self.lock.read_bytes())
        package = next(value for value in lock["packages"].values() if value["name"] == name)
        package.update(payload_sha256=hashlib.sha256(path.read_bytes()).hexdigest(), payload_size=path.stat().st_size)
        self.lock.write_text(json.dumps(lock))
        mapping = json.loads(self.mapping.read_bytes())
        for item in mapping["locked"]:
            item["package"] = lock["packages"][item["key"]]
        self.mapping.write_text(json.dumps(mapping))

    def update_control(self, name, **fields):
        self.control_fields[name].update(fields)
        control = self.controls[name]
        control.write_bytes(control_tar(self.control_fields[name]))
        lock = json.loads(self.lock.read_bytes())
        package = next(value for value in lock["packages"].values() if value["name"] == name)
        package.update(control_sha256=hashlib.sha256(control.read_bytes()).hexdigest(),
                       control_size=control.stat().st_size)
        self.lock.write_text(json.dumps(lock))

    def debug_inputs(self):
        """Model a debug package that needs a runtime addition absent from dpkg status."""
        _, receipt = self.select()
        metadata = self.root / "runtime-selection.json"
        metadata.write_text(json.dumps(receipt))
        write_oci(self.base, [("var/lib/dpkg/status", self.status),
                             ("usr/lib/libssl.so.3", self.base_elf),
                             ("usr/share/new-runtime/data", b"new-runtime")])
        payload = self.root / "debug-tool.tar"
        payload.write_bytes(tar_bytes([("usr/share/debug-tool/data", b"debug")]))
        control = self.root / "debug-tool.control.tar"
        control.write_bytes(control_tar({"Package": "debug-tool", "Version": "1.0",
                                         "Architecture": "amd64", "Depends": "new-runtime (>= 1.0)"}))
        key = "/trixie/debug-tool:amd64=1.0"
        lock = json.loads(self.lock.read_bytes())
        lock["packages"] = {key: {"name": "debug-tool", "version": "1.0", "architecture": "amd64",
                                  "sha256": hashlib.sha256(b"debug-tool").hexdigest(), "depends_on": [],
                                  "payload_sha256": hashlib.sha256(payload.read_bytes()).hexdigest(),
                                  "payload_size": payload.stat().st_size,
                                  "control_sha256": hashlib.sha256(control.read_bytes()).hexdigest(),
                                  "control_size": control.stat().st_size}}
        lock["dependency_sets"] = {"debug": {"sets": {"amd64": {key.rsplit("=", 1)[0]: "1.0"}}}}
        lock_path = self.root / "debug.lock.json"
        lock_path.write_text(json.dumps(lock))
        mapping = self.root / "debug.inputs.json"
        mapping.write_text(json.dumps({"architecture": "amd64", "locked": [
            {"key": key, "payload": str(payload), "control": str(control)}]}))
        return lock_path, mapping, metadata, payload

    def select(self):
        value = json.loads(self.mapping.read_bytes())
        packages = json.loads(self.lock.read_bytes())["packages"]
        for item in value["locked"]:
            item["package"] = packages[item["key"]]
        self.mapping.write_text(json.dumps(value))
        return subject.select(self.base, self.lock, self.policy, self.mapping, variant="runtime")

    def test_public_input_mapping_covers_the_reviewed_lock_closures(self):
        self.assertEqual(json.loads(POLICY.read_bytes()), {
            "schema": 1, "image": "docker-orchagent", "architecture": "amd64",
            "distribution": "trixie", "retained_source": "none",
            "features": {},
        })
        lock = json.loads(LOCK.read_bytes())
        expected = {}
        for group, dependency_set in lock["dependency_sets"].items():
            expected[group] = {}
            for architecture, roots in dependency_set["sets"].items():
                candidates = {}
                pending = [key + "=" + version for key, version in roots.items()]
                while pending:
                    key = pending.pop()
                    if key in candidates:
                        continue
                    package = lock["packages"][key]
                    candidates[key] = "@" + group + "//" + package["name"]
                    pending.extend(package["depends_on"])
                expected[group][architecture] = candidates
        actual = ast.literal_eval(GENERATED_INPUTS.read_text().split("APT_INPUTS =", 1)[1])
        self.assertEqual(actual, expected)

    def test_imported_runtime_and_debug_packages_match_locked_bytes_and_identity(self):
        """Inspect the public import_lock outputs without executing package binaries."""
        lock = json.loads(LOCK.read_bytes())
        mapping = ast.literal_eval(GENERATED_INPUTS.read_text().split("APT_INPUTS =", 1)[1])
        for (group, name), (deb, control) in IMPORTED_PACKAGES.items():
            with self.subTest(group=group, package=name):
                keys = [key for key in mapping[group]["amd64"]
                        if lock["packages"][key]["name"] == name]
                self.assertEqual(len(keys), 1)
                package = lock["packages"][keys[0]]
                self.assertEqual(deb.stat().st_size, package["size"])
                self.assertEqual(hashlib.sha256(deb.read_bytes()).hexdigest(), package["sha256"])
                self.assertEqual(control.stat().st_size, package["control_size"])
                self.assertEqual(hashlib.sha256(control.read_bytes()).hexdigest(), package["control_sha256"])
                with tarfile.open(control, "r:*") as archive:
                    members = [member for member in archive.getmembers()
                               if member.name.removeprefix("./") == "control" and member.isfile()]
                    self.assertEqual(len(members), 1)
                    identity = package_from_control(archive.extractfile(members[0]).read(), origin=str(control))
                self.assertEqual((identity.name, identity.version, identity.architecture),
                                 (package["name"], package["version"], package["architecture"]))

    def test_base_packages_are_retained(self):
        paths, receipt = self.select()
        self.assertEqual(paths, [self.paths["new-runtime"]])
        self.assertEqual([item["package"] for item in receipt["selected"]], ["new-runtime"])
        self.assertEqual(receipt["skipped_base"][0]["base_version"], "3.5.7+fips")
        self.assertEqual(receipt["skipped_base"][0]["selected_version"], "3.5.6")
        self.assertEqual(receipt["skipped_retained"], [])
        self.assertEqual(receipt["image"], "docker-orchagent")
        self.assertEqual(receipt["base_elf_count"], 1)
        self.assertEqual(receipt["dependency_check"]["status"], "satisfied")
        self.assertEqual(receipt["dependency_check"]["package_count"], 2)

    def test_added_package_requires_a_compatible_retained_base_version(self):
        """Reject an addition whose minimum version exceeds the actual FIPS base."""
        self.update_control("new-runtime", Depends="libssl3t64 (>= 3.5.8)")
        with self.assertRaisesRegex(ValueError, "unsatisfied Depends: libssl3t64"):
            self.select()

    def test_unselected_candidate_cannot_satisfy_a_dependency_alternative(self):
        """Do not count the skipped Debian candidate as the installed FIPS version."""
        self.update_control("new-runtime", Depends="libssl3t64 (= 3.5.6) | absent-provider")
        with self.assertRaisesRegex(ValueError, "unsatisfied Depends: libssl3t64"):
            self.select()

    def test_debug_dependencies_use_inherited_runtime_control_metadata(self):
        """Require runtime evidence for dependencies missing from unchanged dpkg status."""
        lock, mapping, metadata, payload = self.debug_inputs()
        with self.assertRaisesRegex(ValueError, "unsatisfied Depends: new-runtime"):
            subject.select(self.base, lock, self.policy, mapping, variant="debug")
        paths, receipt = subject.select(self.base, lock, self.policy, mapping, variant="debug",
                                        base_package_metadata=metadata)
        self.assertEqual(paths, [payload])
        self.assertEqual(receipt["dependency_check"]["package_count"], 3)
        self.assertEqual(receipt["dependency_check"]["packages"]["new-runtime"]["Version"], "1.0")

    def test_selector_cli_forwards_inherited_runtime_metadata(self):
        """Keep dependency evidence available through the selector's command-line path."""
        lock, mapping, metadata, payload = self.debug_inputs()
        output, receipt = self.root / "debug-output", self.root / "debug-selection.json"
        args = ["apt_selection.py", "--base", str(self.base), "--lock", str(lock),
                "--policy", str(self.policy), "--mapping", str(mapping),
                "--variant", "debug", "--base-package-metadata", str(metadata),
                "--out-dir", str(output), "--receipt", str(receipt)]
        with mock.patch.object(sys, "argv", args):
            subject.main()
        self.assertEqual(json.loads(receipt.read_bytes())["dependency_check"]["package_count"], 3)
        self.assertEqual((output / "000001.tar").read_bytes(), payload.read_bytes())

    def test_inherited_metadata_must_match_the_base_package_inventory(self):
        """Reject a runtime receipt from a different dpkg package inventory."""
        lock, mapping, metadata, _ = self.debug_inputs()
        self.status = self.status.replace(b"3.5.7+fips", b"3.5.8+fips")
        self.write_base()
        with self.assertRaisesRegex(ValueError, "does not match the inherited dpkg inventory"):
            subject.select(self.base, lock, self.policy, mapping, variant="debug",
                           base_package_metadata=metadata)

    def test_inherited_metadata_requires_a_successful_dependency_check(self):
        """Reject incomplete or failed runtime dependency evidence."""
        lock, mapping, metadata, _ = self.debug_inputs()
        receipt = json.loads(metadata.read_bytes())
        receipt["dependency_check"]["status"] = "failed"
        metadata.write_text(json.dumps(receipt))
        with self.assertRaisesRegex(ValueError, "lacks a successful dependency check"):
            subject.select(self.base, lock, self.policy, mapping, variant="debug",
                           base_package_metadata=metadata)

    def test_inherited_metadata_cannot_rewrite_an_installed_base_version(self):
        """Reject altered control records even when the receipt keeps the right base hash."""
        lock, mapping, metadata, _ = self.debug_inputs()
        receipt = json.loads(metadata.read_bytes())
        receipt["dependency_check"]["packages"]["libssl3t64"]["Version"] = "9.0"
        metadata.write_text(json.dumps(receipt))
        with self.assertRaisesRegex(ValueError, "changes an installed package control record"):
            subject.select(self.base, lock, self.policy, mapping, variant="debug",
                           base_package_metadata=metadata)

    def test_another_package_cannot_replace_a_base_elf(self):
        self.update_payload("new-runtime", [("usr/lib/libssl.so.3", self.base_elf + b"changed")])
        with self.assertRaisesRegex(ValueError, "would replace a base ELF"):
            self.select()

    def test_identical_base_elf_bytes_are_allowed(self):
        self.update_payload("new-runtime", [("usr/lib/libssl.so.3", self.base_elf)])
        paths, receipt = self.select()
        self.assertEqual(paths, [self.paths["new-runtime"]])
        self.assertEqual(receipt["changed_non_elf_base_paths"], [])

    def test_non_elf_base_changes_are_reported_for_review(self):
        self.update_payload("new-runtime", [("etc/base", b"changed config")])
        _, receipt = self.select()
        self.assertEqual(receipt["changed_non_elf_base_paths"], [{"package": "new-runtime", "path": "etc/base"}])

    def test_changed_data_and_control_bytes_are_rejected(self):
        payload = self.paths["new-runtime"]
        original = payload.read_bytes()
        payload.write_bytes(original + b"changed")
        with self.assertRaisesRegex(ValueError, "changed locked APT payload"):
            self.select()
        payload.write_bytes(original)
        self.controls["new-runtime"].write_text("changed control\n")
        with self.assertRaisesRegex(ValueError, "changed locked APT control"):
            self.select()

    def test_missing_base_status_and_wrong_policy_are_rejected(self):
        self.write_base(status=False)
        with self.assertRaisesRegex(ValueError, "lacks dpkg status"):
            self.select()
        self.write_base()
        value = json.loads(self.policy.read_bytes())
        value["architecture"] = "arm64"
        self.policy.write_text(json.dumps(value))
        with self.assertRaisesRegex(ValueError, "does not match"):
            self.select()

    def test_duplicate_provider_entries_are_rejected(self):
        value = json.loads(self.mapping.read_bytes())
        value["locked"].append(value["locked"][0])
        self.mapping.write_text(json.dumps(value))
        with self.assertRaisesRegex(ValueError, "duplicate package in APT package set"):
            self.select()

    def test_foreign_candidate_package_is_rejected(self):
        lock = json.loads(self.lock.read_bytes())
        next(iter(lock["packages"].values()))["architecture"] = "arm64"
        self.lock.write_text(json.dumps(lock))
        with self.assertRaisesRegex(ValueError, "foreign architecture"):
            self.select()

    def test_identical_package_sources_are_deduplicated(self):
        lock = json.loads(self.lock.read_bytes())
        original_key = next(key for key, value in lock["packages"].items() if value["name"] == "new-runtime")
        duplicate_key = "/trixie-security/new-runtime:amd64=1.0"
        duplicate = dict(lock["packages"][original_key])
        duplicate.update(suite="trixie-security", filename="pool/updates/new-runtime.deb")
        lock["packages"][duplicate_key] = duplicate
        for group in ("runtime", "debug"):
            key, version = duplicate_key.rsplit("=", 1)
            lock["dependency_sets"][group]["sets"]["amd64"][key] = version
        lock["sources"]["trixie-security"] = {"uris": ["https://snapshot.debian.org/archive/debian-security/20260726T121236Z"]}
        self.lock.write_text(json.dumps(lock))
        payload = self.root / "duplicate.tar"
        payload.write_bytes(self.paths["new-runtime"].read_bytes())
        control = self.root / "duplicate.control"
        control.write_bytes(self.controls["new-runtime"].read_bytes())
        mapping = json.loads(self.mapping.read_bytes())
        mapping["locked"].append({"key": duplicate_key, "package": duplicate, "payload": str(payload), "control": str(control)})
        self.mapping.write_text(json.dumps(mapping))
        paths, receipt = self.select()
        self.assertEqual(len(paths), 1)
        self.assertEqual(len(receipt["duplicate_sources"]), 1)
        self.assertEqual(receipt["duplicate_sources"][0]["package"], "new-runtime")

    def test_debug_tools_cannot_change_a_source_built_runtime_library(self):
        # Source-built SWSS payloads need not have a dpkg-status entry. Inspect
        # the completed runtime's actual files as well as its package database.
        write_oci(self.base, [("var/lib/dpkg/status", self.status),
                             ("usr/lib/libswsscommon.so.0", self.base_elf)])
        self.update_payload("new-runtime", [("usr/lib/libswsscommon.so.0", self.base_elf + b"changed")])
        with self.assertRaisesRegex(ValueError, "would replace a base ELF"):
            subject.select(self.base, self.lock, self.policy, self.mapping, variant="debug")

    def test_debug_selection_records_the_exact_runtime_manifest(self):
        _, before = subject.select(self.base, self.lock, self.policy, self.mapping, variant="debug")
        write_oci(self.base, [("var/lib/dpkg/status", self.status), ("etc/runtime-version", b"next")])
        _, after = subject.select(self.base, self.lock, self.policy, self.mapping, variant="debug")
        self.assertNotEqual(before["base_manifest_digest"], after["base_manifest_digest"])
        self.assertEqual(after["group"], "debug")

    def test_whiteouts_in_added_apt_payload_are_rejected(self):
        self.update_payload("new-runtime", [("etc/.wh.base", b"")])
        with self.assertRaisesRegex(ValueError, "contains a whiteout"):
            self.select()

    def test_payload_cannot_traverse_an_inherited_directory_symlink(self):
        output = io.BytesIO()
        with tarfile.open(fileobj=output, mode="w") as archive:
            status = tarfile.TarInfo("var/lib/dpkg/status")
            status.size = len(self.status)
            archive.addfile(status, io.BytesIO(self.status))
            link = tarfile.TarInfo("lib")
            link.type, link.linkname = tarfile.SYMTYPE, "usr/lib"
            archive.addfile(link)
        layer = output.getvalue()
        config = {"architecture": "amd64", "os": "linux",
                  "rootfs": {"type": "layers", "diff_ids": [digest(layer)]}}
        write_layout(self.base, oci_files(json.dumps(config).encode(), [layer]))
        self.update_payload("new-runtime", [("lib/new.so", self.base_elf)])
        with self.assertRaisesRegex(ValueError, "crosses a non-directory"):
            self.select()

    def test_unknown_variant_and_foreign_base_are_rejected(self):
        with self.assertRaisesRegex(ValueError, "unsupported APT variant"):
            subject.select(self.base, self.lock, self.policy, self.mapping, variant="other")
        layer = tar_bytes([("var/lib/dpkg/status", self.status)])
        config = {"architecture": "arm64", "os": "linux",
                  "rootfs": {"type": "layers", "diff_ids": [digest(layer)]}}
        write_layout(self.base, oci_files(json.dumps(config).encode(), [layer]))
        with self.assertRaisesRegex(ValueError, "does not match"):
            self.select()

    def test_executable_hardlink_and_library_symlink_cannot_be_replaced(self):
        elf = {"kind": "file", "elf_machine": 62, "sha256": "a" * 64}
        for kind, target in (("hardlink", "usr/lib/library.so.1"),
                             ("symlink", "library.so.1"),
                             ("symlink", "/usr/lib/library.so.1")):
            with self.subTest(kind=kind, target=target):
                link = {"kind": kind, "linkname": target}
                base = {"usr/lib/library.so.1": elf, "usr/lib/library.so": link}
                assert_overlay_paths({"usr/lib/library.so": link}, base)
                with self.assertRaisesRegex(ValueError, "changes a base ELF link"):
                    assert_overlay_paths({"usr/lib/library.so": {**elf, "sha256": "b" * 64}}, base)
                with self.assertRaisesRegex(ValueError, "changes a base ELF link"):
                    assert_overlay_paths({"usr/lib/library.so": {"kind": "symlink", "linkname": "other.so"}}, base)

    def test_elf_link_resolution_handles_directory_aliases_and_cycles(self):
        base = {"lib": {"kind": "symlink", "linkname": "usr/lib"},
                "usr/lib/library.so.1": {"kind": "file", "elf_machine": 62},
                "usr/lib/library.so": {"kind": "symlink", "linkname": "/lib/library.so.1"},
                "etc/cycle": {"kind": "symlink", "linkname": "cycle"}}
        with self.assertRaisesRegex(ValueError, "changes a base ELF link"):
            assert_overlay_paths({"usr/lib/library.so": {"kind": "symlink", "linkname": "other.so"}}, base)
        assert_overlay_paths({"etc/cycle": {"kind": "file"}}, base)

    def test_runtime_library_directories_and_directory_links_cannot_be_redirected(self):
        base = {"lib": {"kind": "symlink", "linkname": "usr/lib"},
                "usr/lib": {"kind": "directory"},
                "usr/lib/library.so.1": {"kind": "file", "elf_machine": 62}}
        assert_overlay_paths({"lib": base["lib"], "usr/lib": base["usr/lib"]}, base)
        with self.assertRaisesRegex(ValueError, "changes a base directory link"):
            assert_overlay_paths({"lib": {"kind": "symlink", "linkname": "opt/lib"}}, base)
        for kind in ("symlink", "file"):
            with self.subTest(kind=kind), self.assertRaisesRegex(ValueError, "hides a base ELF directory"):
                assert_overlay_paths({"usr/lib": {"kind": kind, "linkname": "elsewhere"}}, base)


if __name__ == "__main__":
    unittest.main()
