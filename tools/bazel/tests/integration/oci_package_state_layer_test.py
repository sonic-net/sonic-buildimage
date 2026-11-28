#!/usr/bin/env python3
"""Check generated package files against their locked owners and link targets."""

import hashlib
import io
import json
from pathlib import Path
import struct
import sys
import tarfile
import tempfile
import unittest

ROOT = Path(__file__).absolute().parents[4]
OWNER = ROOT / "platform/vs/docker-syncd-vs"
sys.path.insert(0, str(ROOT))
from tools.bazel.tests.oci_base_fixture import digest, oci_files, tar_entries as tar_bytes, write_layout
from tools.bazel.oci.oci_inventory import apply_layer
from tools.bazel.tests.oci_contract_fixture import owner_policies, package_policy, source_contract
from tools.bazel.oci import package_state as subject


def write_oci(path, entries, *, symlinks=()):
    layer = tar_bytes(entries)
    if symlinks:
        stream = io.BytesIO(layer)
        with tarfile.open(fileobj=stream, mode="a") as archive:
            for name, target in symlinks:
                member = tarfile.TarInfo(name)
                member.type, member.linkname, member.mode = tarfile.SYMTYPE, target, 0o777
                archive.addfile(member)
        layer = stream.getvalue()
    config = {"architecture": "amd64", "os": "linux",
              "rootfs": {"type": "layers", "diff_ids": [digest(layer)]}}
    files = oci_files(json.dumps(config).encode(), [layer])
    write_layout(path, files)
    return json.loads(files["index.json"])["manifests"][0]["digest"]


class PackageStateLayerTest(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory(prefix="oci-state-test-")
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        self.policy = package_policy()
        self.source_contract = source_contract()
        self.base = self.root / "base.oci"
        base_digest = write_oci(self.base, [("etc/base", b"base", 0o644)])
        self.dockerfile = self.root / "Dockerfile.j2"
        self.dockerfile.write_text("FROM fixture\n")
        self.license = b"sample license\n"
        self.apt = self.root / "apt.tar"
        self.write_apt(self.license)
        self.runtime_layer = self.root / "runtime.tar"
        self.init_bytes = b"#!/bin/sh\nexit 0\n"
        self.runtime_layer.write_bytes(tar_bytes([("etc/init.d/tool", self.init_bytes, 0o755)]))
        key = "/trixie/tool-package:amd64=1.0"
        package = {"name": "tool-package", "version": "1.0", "architecture": "amd64", "suite": "trixie",
                   "filename": "pool/tool.deb", "sha256": "a" * 64, "size": 1, "depends_on": [],
                   "payload_sha256": "b" * 64, "payload_size": 1, "control_sha256": "c" * 64, "control_size": 1}
        self.lock = self.root / "apt.lock.json"
        self.lock.write_text(json.dumps({"version": 2, "facts": {},
            "dependency_sets": {"runtime": {"sets": {"amd64": {key.rsplit("=", 1)[0]: "1.0"}}}},
            "sources": {"trixie": {"uris": ["https://snapshot.debian.org/archive/debian/20260727T143429Z"]}},
            "packages": {key: package}}))
        script_sha = hashlib.sha256(b"postinst").hexdigest()
        self.make_manifest = self.root / "make.json"
        self.make_manifest.write_text(json.dumps({"schema": 1, "image": self.policy["image"], "variant": "runtime",
            "architecture": self.policy["architecture"], "distribution": self.policy["distribution"],
            "features": dict(self.policy["features"]), "packages": [{"package": "make-package", "version": "1.0",
            "architecture": "amd64", "control_fields": {"Depends": "libc6"},
            "control_files": {"postinst": script_sha, "md5sums": "d" * 64}}]}))
        self.selection = self.root / "selection.json"
        self.selection.write_text(json.dumps({"schema": 1, "variant": "runtime", "base_manifest_digest": base_digest,
            "apt_lock_sha256": hashlib.sha256(self.lock.read_bytes()).hexdigest(),
            "make_manifest_sha256": hashlib.sha256(self.make_manifest.read_bytes()).hexdigest(),
            "selected": [{"key": key, "package": "tool-package", "version": "1.0",
                          "control_sha256": package["control_sha256"], "payload_sha256": package["payload_sha256"]}]}))
        state = "auto\n/usr/bin/tool\n\n/usr/bin/tool-1\n10\n\n"
        self.contract = self.root / "state.json"
        self.contract.write_text(json.dumps({"schema": 1,
            "reference": {"buildimage_revision": "1" * 40, "base_archive_sha256": "2" * 64,
                          "runtime_archive_sha256": "3" * 64,
                          "legacy_dockerfile_sha256": hashlib.sha256(self.dockerfile.read_bytes()).hexdigest()},
            "apt_lock_sha256": hashlib.sha256(self.lock.read_bytes()).hexdigest(),
            "package_controls": {"tool-package": {"version": "1.0", "control_sha256": package["control_sha256"]}},
            "package_payloads": {"tool-package": {"version": "1.0", "payload_sha256": package["payload_sha256"]}},
            "make_package_state_inputs": {"make-package": {"version": "1.0", "architecture": "amd64",
                "control_fields": {"Depends": "libc6"}, "maintainer_scripts": {"postinst": script_sha}}},
            "make_package_files": {"etc/init.d/tool": {"package": "make-package",
                "sha256": hashlib.sha256(self.init_bytes).hexdigest(), "size": len(self.init_bytes), "mode": 0o755, "uid": 0, "gid": 0}},
            "entries": [
                {"path": "etc/alternatives/tool", "kind": "symlink", "mode": 0o777, "uid": 0, "gid": 0, "linkname": "/usr/bin/tool-1"},
                {"path": "usr/bin/tool", "kind": "symlink", "mode": 0o777, "uid": 0, "gid": 0, "linkname": "/etc/alternatives/tool"},
                {"path": "etc/rc2.d/S01tool", "kind": "symlink", "mode": 0o777, "uid": 0, "gid": 0, "linkname": "../init.d/tool"},
                {"path": "var/lib/dpkg/alternatives/tool", "kind": "file", "mode": 0o644, "uid": 0, "gid": 0,
                 "text": state, "sha256": hashlib.sha256(state.encode()).hexdigest(), "size": len(state.encode())}],
            "aliases": [{"package": "tool-package", "source": "usr/share/doc/tool-package/copyright",
                "path": "usr/share/doc/tool-old/copyright", "sha256": hashlib.sha256(self.license).hexdigest(),
                "size": len(self.license), "mode": 0o644, "uid": 0, "gid": 0}]}))
        self.output = self.root / "state.tar"

    def write_apt(self, license_data):
        self.apt.write_bytes(tar_bytes([("usr/bin/tool-1", b"#!/bin/sh\nexit 0\n", 0o755),
                                        ("usr/share/doc/tool-package/copyright", license_data, 0o644)]))

    def mutate(self, path, function):
        value = json.loads(path.read_bytes())
        function(value)
        path.write_text(json.dumps(value))

    def update_make_selection(self):
        self.mutate(self.selection, lambda value: value.update(
            make_manifest_sha256=hashlib.sha256(self.make_manifest.read_bytes()).hexdigest()))

    def build(self):
        return subject.build(self.contract, self.lock, self.selection, self.base, self.apt,
                             self.runtime_layer, self.make_manifest, self.output,
                             policy=self.policy, source_contract=self.source_contract)

    def test_checked_links_state_and_alias_are_published(self):
        """Generate reviewed service links, alternative state and license aliases from declared owners."""
        result = self.build()
        files = {}
        apply_layer(self.output, files)
        self.assertEqual(result["links_checked"], 3)
        self.assertEqual(result["aliases"], 1)
        self.assertEqual(files["usr/bin/tool"]["linkname"], "/etc/alternatives/tool")
        self.assertEqual(files["etc/rc2.d/S01tool"]["linkname"], "../init.d/tool")
        self.assertEqual(files["usr/share/doc/tool-old/copyright"]["sha256"], hashlib.sha256(self.license).hexdigest())
        with tarfile.open(self.output, "r:") as archive:
            self.assertTrue(all(member.mtime == 0 for member in archive))

    def test_source_owned_state_needs_no_make_manifest_and_rejects_changed_bytes(self):
        self.policy.update(retained_source="source", features={})
        state = json.loads(self.contract.read_bytes())
        state["make_package_state_inputs"] = {}
        state["source_package_files"] = state.pop("make_package_files")
        state["source_package_files"]["etc/init.d/tool"]["package"] = "libexample"
        self.contract.write_text(json.dumps(state))
        self.make_manifest = None
        self.mutate(self.selection, lambda value: value.update(make_manifest_sha256=None))
        self.assertEqual(self.build()["links_checked"], 3)
        self.runtime_layer.write_bytes(tar_bytes([("etc/init.d/tool", b"changed init", 0o755)]))
        with self.assertRaisesRegex(ValueError, "package state file changed"):
            self.build()

    def test_changed_owner_control_is_rejected(self):
        """Require review when a package supplying generated state changes its control metadata."""
        self.mutate(self.contract, lambda value: value["package_controls"]["tool-package"].update(control_sha256="e" * 64))
        with self.assertRaisesRegex(ValueError, "package state owner changed"):
            self.build()

    def test_selected_owner_must_match_canonical_lock_key(self):
        """Bind a selected state owner to its exact package identity in the canonical lock."""
        self.mutate(self.selection, lambda value: value["selected"][0].update(key="/trixie/other:amd64=1.0"))
        with self.assertRaisesRegex(ValueError, "selected package is absent from the checked lock"):
            self.build()

    def test_unselected_owner_is_rejected(self):
        """Do not generate files on behalf of a package absent from the selected image inputs."""
        self.mutate(self.selection, lambda value: value.update(selected=[]))
        with self.assertRaisesRegex(ValueError, "requires a selected APT package"):
            self.build()

    def test_missing_link_target_is_rejected(self):
        """Reject generated links whose destination is missing from the assembled filesystem."""
        self.mutate(self.contract, lambda value: value["entries"][0].update(linkname="/usr/bin/missing"))
        with self.assertRaisesRegex(ValueError, "link target is absent"):
            self.build()

    def test_source_owned_socket_link_follows_merged_usr_and_checks_exact_unit_bytes(self):
        socket = "usr/lib/systemd/user/example.socket"
        unit = b"[Socket]\nListenStream=%t/example\n"
        self.runtime_layer.write_bytes(tar_bytes([("etc/init.d/tool", self.init_bytes, 0o755),
                                                 (socket, unit, 0o644)]))
        base_digest = write_oci(self.base, [], symlinks=[("lib", "usr/lib")])
        self.mutate(self.selection, lambda value: value.update(base_manifest_digest=base_digest))
        state = json.loads(self.contract.read_bytes())
        state["source_package_files"] = {socket: {
            "package": "libexample", "sha256": hashlib.sha256(unit).hexdigest(),
            "size": len(unit), "mode": 0o644, "uid": 0, "gid": 0,
        }}
        link = "etc/systemd/user/sockets.target.wants/example.socket"
        state["entries"].append({"path": link, "kind": "symlink", "mode": 0o777,
                                 "uid": 0, "gid": 0, "linkname": "/lib/systemd/user/example.socket"})
        self.contract.write_text(json.dumps(state))
        self.build()
        files = {}
        apply_layer(self.output, files)
        self.assertEqual(files[link]["linkname"], "/lib/systemd/user/example.socket")
        self.runtime_layer.write_bytes(tar_bytes([("etc/init.d/tool", self.init_bytes, 0o755),
                                                 (socket, b"unreviewed unit", 0o644)]))
        with self.assertRaisesRegex(ValueError, "package state file changed"):
            self.build()

    def test_changed_alias_bytes_are_rejected(self):
        """An alias must retain the reviewed content of the package that supplies it."""
        self.write_apt(b"different license\n")
        with self.assertRaisesRegex(ValueError, "copyright bytes differ"):
            self.build()

    def test_package_state_cannot_replace_a_base_elf(self):
        """Generated package state must never overwrite an inherited native executable."""
        header = bytearray(64)
        header[:6] = b"\x7fELF\x02\x01"
        struct.pack_into("<HH", header, 16, 3, 62)
        digest = write_oci(self.base, [("usr/bin/tool", bytes(header), 0o755)])
        self.mutate(self.selection, lambda value: value.update(base_manifest_digest=digest))
        with self.assertRaisesRegex(ValueError, "package state would replace an ELF"):
            self.build()

    def test_make_relationship_changes_require_review(self):
        """Ignoring Installed-Size must retain every relationship and unfamiliar future control field."""
        original = self.make_manifest.read_bytes()
        for field in ("Depends", "Pre-Depends", "Provides", "Conflicts", "Breaks", "Replaces",
                      "Recommends", "Suggests", "Enhances", "Multi-Arch", "Essential", "Protected",
                      "X-Future-Relationship"):
            self.make_manifest.write_bytes(original)
            self.mutate(self.make_manifest, lambda value: value["packages"][0]["control_fields"].update(
                {field: "new-value-not-for-the-error-message"}))
            self.update_make_selection()
            with self.subTest(field=field), self.assertRaises(ValueError) as failure:
                self.build()
            self.assertIn("control_fields." + field, str(failure.exception))
            self.assertNotIn("new-value-not-for-the-error-message", str(failure.exception))

    def test_changed_make_identity_or_script_is_rejected(self):
        """Rebuild metadata never permits different package identities or installation scripts."""
        original = self.make_manifest.read_bytes()
        changes = [(None, "version", "2.0"), (None, "architecture", "arm64")]
        changes += [("control_fields", key, "changed") for key in ("Package", "Version", "Architecture", "Source")]
        changes += [("control_files", key, "f" * 64) for key in sorted(subject.SCRIPTS)]
        for section, field, replacement in changes:
            self.make_manifest.write_bytes(original)
            self.mutate(self.make_manifest, lambda value: (
                value["packages"][0] if section is None else value["packages"][0][section]).update({field: replacement}))
            self.update_make_selection()
            diagnostic = ("maintainer_scripts" if section == "control_files" else section)
            diagnostic = diagnostic + "." + field if diagnostic else field
            with self.subTest(field=diagnostic), self.assertRaises(ValueError) as failure:
                self.build()
            self.assertIn(diagnostic, str(failure.exception))

    def test_rebuilt_installed_size_keeps_state_bytes(self):
        """P330's P4C size change, 508420 to 509656 KiB, does not alter generated links or alternatives."""
        self.mutate(self.contract, lambda value: value["make_package_state_inputs"]["make-package"]["control_fields"].update(
            {"Installed-Size": "508420"}))
        self.mutate(self.make_manifest, lambda value: value["packages"][0]["control_fields"].update(
            {"Installed-Size": "508420"}))
        self.update_make_selection()
        self.build()
        before = self.output.read_bytes()
        self.mutate(self.make_manifest, lambda value: value["packages"][0]["control_fields"].update(
            {"Installed-Size": "509656"}))
        self.mutate(self.make_manifest, lambda value: value["packages"][0]["control_files"].update(
            control="cf51e7756f343d470fe66faeec1eb4e9b65e6d326a23926907872b2d60955f4f",
            md5sums="b49cecacbe9965c8ad2c6423755b35b1d9050bfd2c6b59484f9accd7b12f03d0"))
        self.update_make_selection()
        self.build()
        self.assertEqual(self.output.read_bytes(), before)

    def test_missing_or_extra_make_owners_require_review(self):
        """Keep the remaining imported package set exact when source-owned packages leave the handoff."""
        original = self.make_manifest.read_bytes()
        for change in ("missing", "extra"):
            self.make_manifest.write_bytes(original)
            if change == "missing":
                self.mutate(self.make_manifest, lambda value: value.update(packages=[]))
            else:
                self.mutate(self.make_manifest, lambda value: value["packages"].append({"package": "unexpected-package"}))
            self.update_make_selection()
            with self.subTest(change=change), self.assertRaisesRegex(ValueError, "Make package set changed"):
                self.build()

    def test_source_library_cannot_be_reintroduced_as_a_make_state_owner(self):
        """Even matching edited state and manifest records cannot assign source packages back to Make."""
        for package in [record["package"] for record in self.source_contract["packages"]]:
            self.mutate(self.make_manifest, lambda value: value["packages"][0].update(package=package))
            self.mutate(self.contract, lambda value: value.update(make_package_state_inputs={
                package: next(iter(value["make_package_state_inputs"].values()))}))
            self.update_make_selection()
            with self.subTest(package=package), self.assertRaisesRegex(ValueError, "packages now built from source"):
                self.build()

    def test_unrelated_package_md5_changes_keep_state_bytes(self):
        """Ignore unrelated package file-list changes when all state-relevant inputs are unchanged."""
        self.build()
        first = self.output.read_bytes()
        self.mutate(self.make_manifest, lambda value: value["packages"][0]["control_files"].update(md5sums="e" * 64))
        self.update_make_selection()
        self.build()
        self.assertEqual(self.output.read_bytes(), first)

    def test_reviewed_make_archive_identity_cannot_be_substituted(self):
        """Bind explicitly pinned state inputs to their reviewed source and control archives."""
        identity = {"source_sha256": "a" * 64, "control_sha256": "b" * 64}
        self.mutate(self.contract, lambda value: value["make_package_state_inputs"]["make-package"].update(identity))
        self.mutate(self.make_manifest, lambda value: value["packages"][0].update(identity))
        self.update_make_selection()
        self.build()
        original = self.output.read_bytes()
        for field in identity:
            with self.subTest(field=field):
                self.mutate(self.make_manifest, lambda value: value["packages"][0].update(identity))
                self.mutate(self.make_manifest, lambda value: value["packages"][0].update({field: "f" * 64}))
                self.update_make_selection()
                with self.assertRaisesRegex(ValueError, "relationships or scripts changed.*" + field):
                    self.build()
                self.assertEqual(self.output.read_bytes(), original)

    def test_make_state_file_changes_require_review(self):
        """Reject imported service-file changes that differ from the reviewed state contract."""
        self.runtime_layer.write_bytes(tar_bytes([("etc/init.d/tool", b"changed init script\n", 0o755)]))
        with self.assertRaisesRegex(ValueError, "package state file changed"):
            self.build()

    def test_historical_reference_is_not_an_assembly_input(self):
        """Normal assembly uses selected content; historical reference changes are checked separately in CI."""
        self.build()
        before = self.output.read_bytes()
        self.dockerfile.write_text("FROM changed\n")
        self.mutate(self.contract, lambda value: value.update(reference={}, apt_lock_sha256="0" * 64))
        self.build()
        self.assertEqual(self.output.read_bytes(), before)

    def test_unrelated_lock_change_requires_fresh_selection_not_new_state(self):
        """Keep selection bound to the real lock, but do not rebuild state declarations for unrelated lock metadata."""
        self.build()
        before = self.output.read_bytes()
        self.mutate(self.lock, lambda value: value.update(note="changed selection"))
        with self.assertRaisesRegex(ValueError, "APT selection does not match"):
            self.build()
        self.mutate(self.selection, lambda value: value.update(
            apt_lock_sha256=hashlib.sha256(self.lock.read_bytes()).hexdigest()))
        self.build()
        self.assertEqual(self.output.read_bytes(), before)


if __name__ == "__main__":
    OWNER_POLICIES, remaining = owner_policies()
    unittest.main(argv=[__file__, *remaining])
